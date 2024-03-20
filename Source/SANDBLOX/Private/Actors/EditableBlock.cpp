// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/EditableBlock.h"
#include "SANDBLOX/DrawBebugMacros.h"
#include "SANDBLOX/FVectorHelper.h"
#include "Actors/Stud.h"

#include "CompGeom/ConvexHull3.h"
#include "IndexTypes.h"
#include "Math/MathFwd.h"
#include "Math/Box.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AEditableBlock::AEditableBlock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);	
	Vertices = {};
}

// Called when the game starts or when spawned
void AEditableBlock::BeginPlay()
{
	Super::BeginPlay();
}

FVector AEditableBlock::GetVertex(int32 Index)
{
	return Vertices[Index];
}

TArray<FVector> AEditableBlock::GetVertices()
{
	return Vertices;
}

void AEditableBlock::SetVertices(TArray<FVector> NewVertices)
{
	Vertices = NewVertices;
}

void AEditableBlock::AddVertex(FVector NewVertex)
{
	Vertices.Add(NewVertex);
}
void AEditableBlock::SetMaterial(UMaterialInstance* MaterialInstance)
{
	MeshMaterialInstance = MaterialInstance;
	// You can perform any additional operations you need here, such as applying the material to the mesh.
}

bool AEditableBlock::GenerateBody(TArray<FVector> NewVertices, int32 Top)
{
	Mesh->ClearAllMeshSections();

	// Compute the convex hull
	UE::Geometry::FConvexHull3f ConvexHull;
	ConvexHull.bSaveTriangleNeighbors = true;
	bool result = ConvexHull.Solve<FVector3f>(TArray<FVector3f>(NewVertices));
	if (!result) { return false; }

	// Convex hull triangles
	TArray<UE::Geometry::FIndex3i> Triangles = ConvexHull.GetTriangles();

	// Establish face indices and check shape validity
	int32 FaceIdx = 1;
	TArray<int32> FaceIndices;
	bool DownwardsFaceExists = false;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			FVector Normal = FVector(FaceNormal);
			if (!DownwardsFaceExists) {
				if (Normal.X == 0 && Normal.Y == 0 && Normal.Z < 0) {
					DownwardsFaceExists = true;
					FaceIndices.Push(0);
				}
			}
			FaceIndices.Push(FaceIdx++);
		},
		[&](int32 Value) {return UE::Math::TVector<float>(NewVertices[Value]); }
	);
	if (!DownwardsFaceExists) { return false; }
	if (Top > FaceIndices.Num() - 1) { return false; }

	SetVertices(NewVertices);

	// Add each face as a different section to procedural mesh component
	int32 FaceCount = 0;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			FVector Normal = FVector(FaceNormal);
			int32 Section = FaceIndices[FaceCount];

			// Get Vertices for this face, and map them to new face-local IDs
			TMap<int32, int32> GlobalToLocalVertexIDMap;
			TArray<FVector> FaceVertices;
			int32 LocalID = 0;
			for (int32 FaceVertexID : VertexIDs) 
			{ 
				FaceVertices.Add({ this->GetVertex(FaceVertexID) });
				GlobalToLocalVertexIDMap.Add(FaceVertexID, LocalID++);
			}

			// Get Triangles relevant to this face
			TArray<int32> FaceTriangles;
			for (UE::Geometry::FIndex3i Triangle : Triangles) {
				if (GlobalToLocalVertexIDMap.Contains(Triangle.A) &&
					GlobalToLocalVertexIDMap.Contains(Triangle.B) &&
					GlobalToLocalVertexIDMap.Contains(Triangle.C)) {
					int32 FaceTriangle[3] = {
						GlobalToLocalVertexIDMap[Triangle.A],
						GlobalToLocalVertexIDMap[Triangle.B],
						GlobalToLocalVertexIDMap[Triangle.C],
					};
					FaceTriangles.Append(FaceTriangle, 3);
				}
			}

			// Normals
			TArray<FVector> Normals; Normals.Init(Normal, FaceVertices.Num());

			// UV Coordiantes
			TArray<FVector2D> UV0;
			for (int32 FaceIndex = 0; FaceIndex < FaceVertices.Num(); FaceIndex++)
			{
				// Determine a local 2D coordinate system for each face
				FVector FaceTangent = FVector::CrossProduct(Normal, FVector::UpVector);
				FVector FaceBitangent = FVector::CrossProduct(Normal, FaceTangent);
				FVector2D UVCoords;

				// Project vertices onto the local 2D coordinate system
				FVector LocalVertex = FVector::VectorPlaneProject(FaceVertices[FaceIndex], Normal);
				float U = FVector::DotProduct(LocalVertex, FaceTangent);
				float V = FVector::DotProduct(LocalVertex, FaceBitangent);
				UVCoords.X = U;
				UVCoords.Y = V;

				// Normalize UV coordinates if necessary
				UVCoords.Normalize();
				UV0.Add(UVCoords);
			}

			TArray<FColor> VertexColors;// Empty vertex colors
			TArray<FProcMeshTangent> Tangents; // Empty tangents

			// Create Mesh Section for this face
			Mesh->CreateMeshSection(Section, FaceVertices, FaceTriangles, Normals, UV0, VertexColors, Tangents, true);
			Mesh->SetMaterial(Section, MeshMaterialInstance);

			// Add Studs
			if (Section == Top) {
				FVector Center = AvergePosition(FaceVertices);

				// Studs halfway between center of face and vertex
				for (FVector Vertex : FaceVertices) {
					UStud* NewStud = NewObject<UStud>(this, UStud::StaticClass());
					NewStud->RegisterComponent();
					NewStud->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
					NewStud->CreationMethod = EComponentCreationMethod::Instance;
					NewStud->SetRelativeLocation(AvergePosition({Center, Vertex}));
					NewStud->AddRelativeRotation(Normal.Rotation() + FRotator(-90, 0, 0));
					NewStud->SetMaterial(0, MeshMaterialInstance);
				}

				// Stud at center of face
				UStud* NewStud = NewObject<UStud>(this, UStud::StaticClass());
				NewStud->RegisterComponent();
				NewStud->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
				NewStud->CreationMethod = EComponentCreationMethod::Instance;
				NewStud->SetRelativeLocation(Center);
				NewStud->AddRelativeRotation(Normal.Rotation() + FRotator(-90, 0, 0));
				NewStud->SetMaterial(0, MeshMaterialInstance);
			}
			
			FaceCount++;
		},
		[&](int32 Value) {return UE::Math::TVector<float>(this->GetVertex(Value)); }
	);
	return true;
}

// Called every frame
void AEditableBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

