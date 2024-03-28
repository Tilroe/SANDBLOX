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

	UFUNCTION(BlueprintPure)
	int GetXFactor();

	UFUNCTION(BlueprintPure)
	int GetYFactor();

	UFUNCTION(BlueprintPure)
	int GetZFactor();

	UFUNCTION(BlueprintPure)
	int GetXPivot();

	UFUNCTION(BlueprintPure)
	int GetYPivot();

	UFUNCTION(BlueprintPure)
	int GetTop();

	UFUNCTION(BlueprintCallable)
	void SetVertices(TArray<FVector> NewVertices);

	UFUNCTION(BlueprintCallable)
	void AddVertex(FVector NewVertex);

	UFUNCTION(BlueprintCallable)
	void SetMaterial(UMaterialInstance* MaterialInstance);

	void AddStud(FVector Location, FVector Normal);

private:
	UFUNCTION(BlueprintCallable)
	bool GenerateBody(int XFactor, int YFactor, int ZFactor, int XPivot, int YPivot, int32 Top);

	class UProceduralMeshComponent *Mesh;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	TArray<FVector> Vertices;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 XFactor = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 YFactor = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 ZFactor = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 XPivot = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 YPivot = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Shape")
	int32 Top = 0;

	UPROPERTY(EditAnywhere, Category = "Material")
	UMaterialInstance* MeshMaterialInstance;
};
