// Fill out your copyright notice in the Description page of Project Settings.


#include "BearPushAbility.h"
#include "MyPlayerCharacter.h"
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

	if (!bIsPushing) 
	{
		TryStartPush();
	}
	else 
	{
		StopPush();
	}
	return true;
}

void UBearPushAbility::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!bIsPushing)
	{
		return;
	}

	if (!CachedPlayer || !CurrentPushActor)
	{
		return;
	}

	FVector PlayerDelta = CachedPlayer->GetVelocity() * DeltaTime;

	CurrentPushActor->AddActorWorldOffset(PlayerDelta,true);
}




void UBearPushAbility::TryStartPush()
{
	if (!CachedPlayer) return;

	APushableActor* Target = FindPushableActor();

	if (!Target) return;
	if (!Target->CanBePushed()) return;

	CurrentPushActor = Target;

	bIsPushing = true;

	CachedPlayer->CurrentMorphData->bCanJump = false;

	Target->BeginPush(CachedPlayer);

	// 玩家移动到推动点
	CachedPlayer->SetActorLocation(Target->PushPoint->GetComponentLocation());

	CachedPlayer->SetActorRotation(Target->PushPoint->GetComponentRotation());

	// 锁定旋转
	CachedPlayer->bUseControllerRotationYaw = false;

	auto* MoveComp = CachedPlayer->GetCharacterMovement();

	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = PushSpeed;
		MoveComp->bOrientRotationToMovement = false;
	}


	// 然后播放推动动画
}

void UBearPushAbility::StopPush()
{
	if (!bIsPushing)
	{
		return;
	}

	bIsPushing = false;

	CachedPlayer->CurrentMorphData->bCanJump = true;

	if (CurrentPushActor)
	{
		CurrentPushActor->EndPush();
	}


	if (CachedPlayer)
	{
		auto* MoveComp = CachedPlayer->GetCharacterMovement();

		if (MoveComp)
		{
			MoveComp->bOrientRotationToMovement = true;
			MoveComp->MaxWalkSpeed = CachedPlayer->CurrentMorphData->MaxWalkSpeed;
		}
	}

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


void UBearPushAbility::OnAbilityAdded_Implementation()
{
	Super::OnAbilityAdded_Implementation();
	UE_LOG(LogTemp, Warning,TEXT("Bear Ability Added"));
}

void UBearPushAbility::OnAbilityRemoved_Implementation()
{
	Super::OnAbilityRemoved_Implementation();

	StopPush();
}