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
	Super::TickComponent(
		DeltaTime,
		TickType,
		ThisTickFunction);
}



void UBearPushAbility::TryStartCarry()
{
	if (!CachedPlayer) return;

	APushableActor* Target = FindPushableActor();

	if (!Target) return;
	if (!Target->CanBePushed()) return;

	CurrentCarryActor = Target;

	bIsCarrying = true;

	auto* MoveComp = CachedPlayer->GetCharacterMovement();

	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = CarrySpeed;
	}

	UPrimitiveComponent* Collision = Target->ActorCollision;

	if (Collision)
	{
		bCachedPhysics = Collision->IsSimulatingPhysics();

		Collision->SetSimulatePhysics(false);

		Collision->IgnoreActorWhenMoving(CachedPlayer, true);

		CachedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(Target, true);
	}

	// Attach 到 CarryPoint
	Target->AttachToComponent(CachedPlayer->CarryPoint,FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	Target->BeginCarry(CachedPlayer);

	// 然后播放推动动画
}

void UBearPushAbility::StopCarry()
{
	if (!bIsCarrying)
	{
		return;
	}

	bIsCarrying = false;

	if (CurrentCarryActor)
	{
		CurrentCarryActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		UPrimitiveComponent* Collision = CurrentCarryActor->ActorCollision;

		if (Collision)
		{
			Collision->SetSimulatePhysics(bCachedPhysics);

			Collision->IgnoreActorWhenMoving(CachedPlayer, false);

			CachedPlayer->GetCapsuleComponent()->IgnoreActorWhenMoving(CurrentCarryActor, false);
		}


		CurrentCarryActor->EndCarry();
	}


	if (CachedPlayer)
	{
		auto* MoveComp = CachedPlayer->GetCharacterMovement();

		if (MoveComp)
		{
			MoveComp->MaxWalkSpeed = CachedPlayer->CurrentMorphData->MaxWalkSpeed;
		}
	}


	CurrentCarryActor = nullptr;
	// 接下来恢复正常动画
}


APushableActor* UBearPushAbility::FindPushableActor()
{
	if (!CachedPlayer)
	{
		return nullptr;
	}

	FVector Start = CachedPlayer->GetActorLocation();

	FVector End = Start + CachedPlayer->GetActorForwardVector() * CheckDistance;

	FHitResult Hit;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility
	);

	if (!bHit)
	{
		return nullptr;
	}

	return Cast<APushableActor>(Hit.GetActor());
}

bool UBearPushAbility::IsCarryBlocked(FVector MoveDirection) const
{
	if (!bIsCarrying)
	{
		return false;
	}

	if (!CurrentCarryActor)
	{
		return false;
	}

	FVector Start = CurrentCarryActor->ActorCollision->GetComponentLocation();

	FVector End = Start + MoveDirection * 50.f;

	FCollisionShape Shape = CurrentCarryActor->ActorCollision->GetCollisionShape();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CachedPlayer);
	Params.AddIgnoredActor(CurrentCarryActor);

	FHitResult Hit;

	bool bBlocked =
		GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			ECC_GameTraceChannel1,
			Shape,
			Params
		);

	return bBlocked;
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