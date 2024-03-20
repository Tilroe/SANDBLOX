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

	UFUNCTION(BlueprintPure)
	FVector GetVertex(int32 Index);

	UFUNCTION(BlueprintPure)
	TArray<FVector> GetVertices();

	UFUNCTION(BlueprintCallable)
	void SetVertices(TArray<FVector> NewVertices);

	UFUNCTION(BlueprintCallable)
	void AddVertex(FVector NewVertex);

	UFUNCTION(BlueprintCallable)
	void SetMaterial(UMaterialInstance* MaterialInstance);

private:
	UFUNCTION(BlueprintCallable)
	bool GenerateBody(TArray<FVector> NewVertices, int32 Top);

	class UProceduralMeshComponent *Mesh;

	UPROPERTY(EditInstanceOnly, Category = "Shape")
	TArray<FVector> Vertices;

	UPROPERTY(EditAnywhere, Category = "Material")
	UMaterialInstance* MeshMaterialInstance;
};
