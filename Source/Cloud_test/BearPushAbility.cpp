// Fill out your copyright notice in the Description page of Project Settings.


#include "BearPushAbility.h"
#include "MyPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UBearPushAbility::UBearPushAbility()
{
	PrimaryComponentTick.bCanEverTick = true; 
}

bool UBearPushAbility::HandleAbilityInputPressed_Implementation(int32 SlotIndex)
{
	if (SlotIndex != 0) {
		return false;
	}

	if (!bIsCarrying) 
	{
		TryStartCarry();
	}
	else 
	{
		StopCarry();
	}
	return true;
}

void UBearPushAbility::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
}



void UBearPushAbility::TryStartCarry()
{
	if (!CachedPlayer) return;

	APushableActor* Target = Cast<APushableActor>(CachedPlayer->CurrentInteractable);;

	if (!Target) return;
	if (!Target->CanBePushed()) return;

	CurrentCarryActor = Target;

	bIsCarrying = true;

	FVector OriginalScale = Target->GetActorScale3D();

	Target->AttachToComponent(CachedPlayer->CarryPoint,FAttachmentTransformRules::KeepRelativeTransform);
	Target->SetActorRelativeLocation(Target->CarryOffset);
	Target->SetActorScale3D(OriginalScale);
	Target->SetActorEnableCollision(false);

	CachedPlayer->HideInteractPrompt();
}

void UBearPushAbility::StopCarry()
{
	if (!bIsCarrying) return;

	FVector DropLocation = CachedPlayer->GetActorLocation() + CachedPlayer->GetActorForwardVector() * CurrentCarryActor->OffsetDistance;

	CurrentCarryActor->SetActorLocation(DropLocation);

	CurrentCarryActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	CurrentCarryActor->SetActorEnableCollision(true);

	CachedPlayer->ShowInteractPrompt(CurrentCarryActor->GetInteractionText_Implementation());

	bIsCarrying = false;

	CurrentCarryActor = nullptr;
}



void UBearPushAbility::OnAbilityAdded_Implementation()
{
	Super::OnAbilityAdded_Implementation();
	UE_LOG(LogTemp, Warning,TEXT("Bear Ability Added"));
}

void UBearPushAbility::OnAbilityRemoved_Implementation()
{
	Super::OnAbilityRemoved_Implementation();

	StopCarry();
}