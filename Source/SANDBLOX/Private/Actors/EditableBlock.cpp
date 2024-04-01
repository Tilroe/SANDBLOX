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

#include <algorithm>

const int32 GridSize = 40;

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

int AEditableBlock::GetXFactor()
{
	return XFactor;
}

int AEditableBlock::GetYFactor()
{
	return YFactor;
}

int AEditableBlock::GetZFactor()
{
	return ZFactor;
}

int AEditableBlock::GetXPivot()
{
	return XPivot;
}

int AEditableBlock::GetYPivot()
{
	return YPivot;
}

int AEditableBlock::GetTop()
{
	return Top;
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

void AEditableBlock::AddStud(FVector Location, FVector Normal)
{
	UStud* NewStud = NewObject<UStud>(this, UStud::StaticClass());
	NewStud->RegisterComponent();
	NewStud->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	NewStud->CreationMethod = EComponentCreationMethod::Instance;
	NewStud->SetRelativeLocation(Location);
	NewStud->AddRelativeRotation(Normal.Rotation() + FRotator(-90, 0, 0));
	NewStud->SetMaterial(0, MeshMaterialInstance);
}

bool AEditableBlock::GenerateBody(int NewXFactor, int NewYFactor, int NewZFactor, int NewXPivot, int NewYPivot, int32 NewTop)
{
	Mesh->ClearAllMeshSections();

	XFactor = NewXFactor;
	YFactor = NewYFactor;
	ZFactor = NewZFactor;
	XPivot = NewXPivot;
	YPivot = NewYPivot;
	Top = NewTop;

	// Pivot Check
	if (XPivot > XFactor ||
		XPivot < 1 ||
		YPivot > YFactor ||
		YPivot < 1) {
		return false;
	}

	// Top Check
	if (Top > 5 || Top < 1) { return false; }

	// Compute Vertices
	int XBig = (XFactor - XPivot) * GridSize + GridSize/2;
	int XSmall = XBig - XFactor * GridSize;
	int YBig = (YFactor - YPivot) * GridSize + GridSize / 2;
	int YSmall = YBig - YFactor * GridSize;
	int ZBig = ZFactor * GridSize;
	int ZSmall = 0;
	TArray<FVector> NewVertices = {
		FVector(XSmall, YSmall, ZSmall),
		FVector(XBig,	YSmall, ZSmall),
		FVector(XSmall, YBig,	ZSmall),
		FVector(XBig,	YBig,	ZSmall),
		FVector(XSmall, YSmall, ZBig),
		FVector(XBig,	YSmall, ZBig),
		FVector(XSmall, YBig,	ZBig),
		FVector(XBig,	YBig,	ZBig),
	};

	// Compute the convex hull
	UE::Geometry::FConvexHull3f ConvexHull;
	ConvexHull.bSaveTriangleNeighbors = true;
	bool result = ConvexHull.Solve<FVector3f>(TArray<FVector3f>(NewVertices));
	if (!result) { return false; }

	// Convex hull triangles
	TArray<UE::Geometry::FIndex3i> Triangles = ConvexHull.GetTriangles();

	// Establish face indices (0 = bottom)
	int32 FaceIdx = 1;
	TArray<int32> FaceIndices;
	TMap<FVector, int32> NormMapping;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			FVector Normal = FVector(FaceNormal);
			if (Normal.Z < 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 0)); }
			else if (Normal.Z > 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 5)); }
			else if (Normal.X > 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 1)); }
			else if (Normal.Y < 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 2)); }
			else if (Normal.X < 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 3)); }
			else if (Normal.Y > 0) { NormMapping.Add(TTuple<FVector, int32>(Normal, 4)); }
		},
		[&](int32 Value) {return UE::Math::TVector<float>(NewVertices[Value]); }
	);

	// Set new vertices (not really needed anymore)
	SetVertices(NewVertices);

	// Add each face as a different section to procedural mesh component
	int32 FaceCount = 0;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			FVector Normal = FVector(FaceNormal);
			int32 Section = NormMapping[Normal];

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
				if (Normal.X != 0) {
					for (int y = YSmall + GridSize / 2; y <= YBig - GridSize / 2; y += GridSize) {
						for (int z = GridSize / 2; z <= ZBig - GridSize / 2; z += GridSize) {
							AddStud(FVector(FaceVertices[0].X, y, z), Normal);
						}
					}
				}
				else if (Normal.Y != 0) {
					for (int x = XSmall + GridSize / 2; x <= XBig - GridSize / 2; x += GridSize) {
						for (int z = GridSize / 2; z <= ZBig - GridSize / 2; z += GridSize) {
							AddStud(FVector(x, FaceVertices[0].Y, z), Normal);
						}
					}
				}
				else {
					for (int x = XSmall + GridSize / 2; x <= XBig - GridSize / 2; x += GridSize) {
						for (int y = YSmall + GridSize / 2; y <= YBig - GridSize / 2; y += GridSize) {
							AddStud(FVector(x, y, FaceVertices[0].Z), Normal);
						}
					}
				}
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

