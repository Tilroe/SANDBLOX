// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawns/PlayerPawn.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

// Sets default values
APlayerPawn::APlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetCapsuleRadius(10.f);
	Capsule->SetCapsuleHalfHeight(10.f);
	SetRootComponent(Capsule);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Capsule);
	SpringArm->TargetArmLength = 0.f;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(SpringArm);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void APlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void APlayerPawn::Forward(float Value)
{
	if (Controller && Value) 
	{
		FVector ForwardVector = GetActorForwardVector();
		AddMovementInput(ForwardVector, Value);
	}
}

void APlayerPawn::Right(float Value)
{
	if (Controller && Value)
	{
		FVector RightVector = GetActorRightVector();
		AddMovementInput(RightVector, Value);
	}
}

void APlayerPawn::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void APlayerPawn::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

// Called every frame
void APlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("Forward"), this, &APlayerPawn::Forward);
	PlayerInputComponent->BindAxis(FName("Right"), this, &APlayerPawn::Right);
	PlayerInputComponent->BindAxis(FName("Turn"), this, &APlayerPawn::Turn);
	PlayerInputComponent->BindAxis(FName("LookUp"), this, &APlayerPawn::LookUp);
}

