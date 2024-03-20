// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Stud.h"

UStud::UStud()
{
	UStaticMesh* StudMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Blueprints/Actors/SM_Stud.SM_Stud"));
	if (StudMesh) {
		SetStaticMesh(StudMesh);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Failed to load stud mesh"));
	}
	
}
