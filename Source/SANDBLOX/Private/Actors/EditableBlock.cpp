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

const int32 GridSize = 20;

// Helper Functions

// Helper function to calculate the centroid of a polygon
FVector CalculateCentroid(const TArray<FVector>& Points) {
	FVector Centroid(0, 0, 0);
	for (const FVector& Point : Points) {
		Centroid += Point;
	}
	Centroid /= Points.Num();
	return Centroid;
}

// Comparator function for sorting points by angle
bool SortByAngle(const FVector& A, const FVector& B, const FVector& Centroid) {
	FVector DirA = A - Centroid;
	FVector DirB = B - Centroid;
	float AngleA = FMath::Atan2(DirA.Y, DirA.X);
	float AngleB = FMath::Atan2(DirB.Y, DirB.X);
	return AngleA < AngleB;
}

// Function to order points of a convex polygon
TArray<FVector> OrderPoints(TArray<FVector> Points) {
	FVector Centroid = CalculateCentroid(Points);
	Points.Sort([&](const FVector& A, const FVector& B) {
		return SortByAngle(A, B, Centroid);
		});
	return Points;
}

bool IsPointInConvexPolygon(const TArray<FVector>& PolygonPoints, const FVector& P) {
	// Assuming PolygonPoints.size() > 2 and they form a convex polygon
	FVector Normal = FVector::CrossProduct(PolygonPoints[1] - PolygonPoints[0], PolygonPoints[2] - PolygonPoints[0]).GetSafeNormal();

	for (int i = 0; i < PolygonPoints.Num(); ++i) {
		FVector A = PolygonPoints[i];
		FVector B = PolygonPoints[(i + 1) % PolygonPoints.Num()];
		FVector Edge = B - A;
		FVector PointToEdgeStart = P - A;
		FVector EdgeDirection = Edge.GetSafeNormal();

		// Project PointToEdgeStart onto Edge to find the closest point on the line extended from Edge
		float ProjectionLength = FVector::DotProduct(PointToEdgeStart, EdgeDirection);
		FVector ClosestPoint;
		if (ProjectionLength < 0) {
			// Closest to A
			ClosestPoint = A;
		}
		else if (ProjectionLength > Edge.Size()) {
			// Closest to B
			ClosestPoint = B;
		}
		else {
			// Closest point lies within the edge segment
			ClosestPoint = A + EdgeDirection * ProjectionLength;
		}

		// Calculate distance from P to the closest point on the edge
		float Distance = (P - ClosestPoint).Size();

		FVector CrossProduct = FVector::CrossProduct(PointToEdgeStart, Edge);

		// Modify condition to consider the point outside if it is within 5 units of an edge
		if (FVector::DotProduct(CrossProduct, Normal) > 0 || Distance <= 5.0f) {
			return false;
		}
	}

	// If P passes all edge tests and is not within 5 units of any edge, it is inside the polygon
	return true;
}


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

				// GridUp and GridRight define the axes/plane to create studs on
				FVector GridUp, GridRight;
				if (Normal.Z == 0) { 
					GridUp = FVector::UpVector; 
					GridRight = FVector::CrossProduct(GridUp, Normal);
				}
				else {
					GridUp = FVector::VectorPlaneProject(FVector(1, 0, 0), Normal);
					GridRight = FVector::VectorPlaneProject(FVector(0, 1, 0), Normal);
				}
				bool r1 = GridUp.Normalize();
				bool r2 = GridRight.Normalize();

				// Order the vertices to create a closed loop for point-in-polygon algorithm
				TArray<FVector> OrderedVertices = OrderPoints(FaceVertices);
				
				// Spawn studs in rings until there is a ring we can no longer spawn studs on
				int RingLevel = 0;
				bool RingValid = true;
				FVector pLoc;

				while (RingValid) {
					RingValid = false;
					for (int x = -RingLevel; x <= RingLevel; ++x) {
						pLoc = Center + (x * GridRight * GridSize) + (RingLevel * GridUp * GridSize);
						if (IsPointInConvexPolygon(OrderedVertices, pLoc)) { AddStud(pLoc, Normal); RingValid = true; }

						if (RingLevel > 0) {
							pLoc = Center + (x * GridRight * GridSize) + (-RingLevel * GridUp * GridSize);
							if (IsPointInConvexPolygon(OrderedVertices, pLoc)) { AddStud(pLoc, Normal); RingValid = true; }

							if (x != -RingLevel && x != RingLevel) {
								pLoc = Center + (RingLevel * GridRight * GridSize) + (x * GridUp * GridSize);
								if (IsPointInConvexPolygon(OrderedVertices, pLoc)) { AddStud(pLoc, Normal); RingValid = true; }

								pLoc = Center + (-RingLevel * GridRight * GridSize) + (x * GridUp * GridSize);
								if (IsPointInConvexPolygon(OrderedVertices, pLoc)) { AddStud(pLoc, Normal); RingValid = true; }
							}
						}
					}
					RingLevel++;
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

