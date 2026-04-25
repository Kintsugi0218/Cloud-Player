// Fill out your copyright notice in the Description page of Project Settings.


#include "MyNpc.h"
#include "GameFramework/CharacterMovementComponent.h"

AMyNpc::AMyNpc()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
}


void AMyNpc::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMyNpc::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMyNpc::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

