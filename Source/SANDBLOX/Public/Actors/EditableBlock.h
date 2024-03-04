// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EditableBlock.generated.h"


UCLASS()
class SANDBLOX_API AEditableBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEditableBlock();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	FVector getVertex(int32 Index);

private:
	bool GenerateBody();

	TArray<FVector> Vertices;
	class UProceduralMeshComponent *Mesh;	
};
