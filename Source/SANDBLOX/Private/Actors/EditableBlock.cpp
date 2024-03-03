// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/EditableBlock.h"
#include "SANDBLOX/DrawBebugMacros.h"

#include "CompGeom/ConvexHull3.h"
#include "IndexTypes.h"
#include "Math/MathFwd.h"
#include "ProceduralMeshComponent.h"

void ProcessPolygonFace(const TArray<int32> VertexIDs, const FVector3f FaceNormal)
{
	// Generate mesh geometry for the current polygon face
	// Create vertices, define triangles, compute normals, etc.

	// Example: Print vertex IDs and normal of the current face
	for (const FVertexID& VertexID : VertexIDs)
	{
		// Process each vertex ID
		// Example: Print vertex ID
		UE_LOG(LogTemp, Warning, TEXT("Vertex ID: %d"), VertexID.GetValue());
	}

	// Example: Print normal of the current face
	UE_LOG(LogTemp, Warning, TEXT("Face Normal: %s"), *(FaceNormal.ToString()));
}


TArray<int32> GetFaceTriangles(TArray<UE::Geometry::FIndex3i> Triangles, TArray<int32> FaceVertexIDs) {
	TArray<int32> FaceTriangles; // Local vertex IDs defining triangles
	TMap<int32, int32> VertexMap; // Mapping global vertex ID to local Face vertex ID
	int32 VertexCount = 0;

	// Check each triangle to see if in face
	for (UE::Geometry::FIndex3i Triangle : Triangles) {
		if (FaceVertexIDs.Contains(Triangle.A) && FaceVertexIDs.Contains(Triangle.B) && FaceVertexIDs.Contains(Triangle.C)) {
			if (!VertexMap.Contains(Triangle.A)) { VertexMap.Add(Triangle.A, VertexCount); VertexCount++; }
			if (!VertexMap.Contains(Triangle.B)) { VertexMap.Add(Triangle.B, VertexCount); VertexCount++; }
			if (!VertexMap.Contains(Triangle.C)) { VertexMap.Add(Triangle.C, VertexCount); VertexCount++; }

			int32 v[3] = { VertexMap[Triangle.A], VertexMap[Triangle.B], VertexMap[Triangle.C] };
			FaceTriangles.Append(v, 3);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("NEW FACE!"));

	UE_LOG(LogTemp, Warning, TEXT("Global Vertex IDs:"));
	for (int32 id : FaceVertexIDs) { UE_LOG(LogTemp, Warning, TEXT("%d"), id); }

	UE_LOG(LogTemp, Warning, TEXT("Global Triangles:"));
	for (UE::Geometry::FIndex3i Triangle : Triangles) { UE_LOG(LogTemp, Warning, TEXT("%d, %d, %d"), Triangle.A, Triangle.B, Triangle.C); }

	UE_LOG(LogTemp, Warning, TEXT("Vertex Mapping:"));
	for (auto Pair : VertexMap) { UE_LOG(LogTemp, Warning, TEXT("%d -> %d"), Pair.Key, Pair.Value); }

	UE_LOG(LogTemp, Warning, TEXT("Face Triangles:"));
	for (int i = 0; i < FaceTriangles.Num(); i += 3) {
		UE_LOG(LogTemp, Warning, TEXT("%d, %d, %d"), FaceTriangles[i], FaceTriangles[i+1], FaceTriangles[i+2]);
	}

	// Offset indeces by minimum index, 
	return FaceTriangles;
};

// Sets default values
AEditableBlock::AEditableBlock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Vertices = { 
		FVector3f(0, 0, 0),
		FVector3f(10.f, 0, 0),
		FVector3f(0, 10.f, 0),
		FVector3f(10.f, 10.f, 0),
		FVector3f(0, 0, 10.f),
		FVector3f(10.f, 0, 10.f),
		FVector3f(0, 10.f, 10.f),
		FVector3f(10.f, 10.f, 10.f),
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

FVector3f AEditableBlock::getVertex(int32 Index)
{
	return Vertices[Index];
}

void AEditableBlock::GenerateBody()
{
	// Compute the convex hull
	UE::Geometry::FConvexHull3f ConvexHull;
	ConvexHull.bSaveTriangleNeighbors = true;

	bool result = ConvexHull.Solve<FVector3f>(Vertices);
	UE_LOG(LogTemp, Display, TEXT("Convex Hull Solve Result: %d"), result);

	// Triangles
	TArray<UE::Geometry::FIndex3i> Triangles = ConvexHull.GetTriangles();
	for (UE::Geometry::FIndex3i tri : Triangles)
	{
		UE_LOG(LogTemp, Display, TEXT("Triangle: %i, %i, %i"), tri.A, tri.B, tri.C);
	}

	// Add each face as a different section to procedural mesh component
	int32 FaceCount = 0;
	ConvexHull.GetFaces(
		[&](const TArray<int32> VertexIDs, const FVector3f FaceNormal)
		{
			// Get Vertices for this face
			TArray<FVector> FaceVertices;
			for (int32 FaceVertexID : VertexIDs) {
				FVector3f v = this->getVertex(FaceVertexID);
				FaceVertices.Add({ FVector(v.X, v.Y, v.Z) });
			}

			// Assuming you don't have these ready, creating dummy arrays
			TArray<int32> FaceTriangles = GetFaceTriangles(Triangles, VertexIDs);
			TArray<FVector> Normals; // Empty, since we're not computing them here
			TArray<FVector2D> UV0; // Empty UV coordinates
			TArray<FColor> VertexColors; // Empty vertex colors
			TArray<FProcMeshTangent> Tangents; // Empty tangents

			// CreateMeshSection has more parameters, added dummy ones for demonstration
			this->Mesh->CreateMeshSection(FaceCount, FaceVertices, FaceTriangles, Normals, UV0, VertexColors, Tangents, false);
			FaceCount++;
		},
		[&](int32 Value) {return this->getVertex(Value); }
	);
}

// Called every frame
void AEditableBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//FVector3f Location = GetActorLocation();
	//for (FVector3f Vertex : Vertices) {	
	//	DRAW_POINT_SingleFrame(Location + Vertex);
	//}

}

