// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/EditableBlock.h"
#include "SANDBLOX/DrawBebugMacros.h"

#include "CompGeom/ConvexHull3.h"
#include "IndexTypes.h"
#include "Math/MathFwd.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AEditableBlock::AEditableBlock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Vertices = {
		FVector(-5.f, -5.f, -5.f),
		FVector(5.f, -5.f, -5.f),
		FVector(-5.f, 5.f, -5.f),
		FVector(5.f, 5.f, -5.f),
		FVector(-5.f, -5.f, 5.f),
		FVector(5.f, -5.f, 5.f),
		FVector(-5.f, 5.f, 5.f),
		FVector(5.f, 5.f, 5.f),
		FVector(0, 0, 10)
	};
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);	
}

// Called when the game starts or when spawned
void AEditableBlock::BeginPlay()
{
	Super::BeginPlay();
	GenerateBody();
}

FVector AEditableBlock::getVertex(int32 Index)
{
	return Vertices[Index];
}

bool AEditableBlock::GenerateBody()
{
	// Compute the convex hull
	UE::Geometry::FConvexHull3f ConvexHull;
	ConvexHull.bSaveTriangleNeighbors = true;
	bool result = ConvexHull.Solve<FVector3f>(TArray<FVector3f>(Vertices));
	if (!result) { return false; }

	// Convex hull triangles
	TArray<UE::Geometry::FIndex3i> Triangles = ConvexHull.GetTriangles();

	// Add each face as a different section to procedural mesh component
	int32 FaceCount = 0;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			FVector Normal = FVector(FaceNormal);

			// Get Vertices for this face, and map them to new face-local IDs
			TMap<int32, int32> GlobalToLocalVertexIDMap;
			TArray<FVector> FaceVertices;
			int32 LocalID = 0;
			for (int32 FaceVertexID : VertexIDs) 
			{ 
				FaceVertices.Add({ this->getVertex(FaceVertexID) });
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

			TArray<FColor> VertexColors; // Empty vertex colors
			TArray<FProcMeshTangent> Tangents; // Empty tangents

			// Create Mesh Section for this face
			Mesh->CreateMeshSection(FaceCount, FaceVertices, FaceTriangles, Normals, UV0, VertexColors, Tangents, false);
			Mesh->SetMaterial(FaceCount, DefaultMaterial);

			FaceCount++;
		},
		[&](int32 Value) {return UE::Math::TVector<float>(this->getVertex(Value)); }
	);

	return true;
}

// Called every frame
void AEditableBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEditableBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	GenerateBody();
}

