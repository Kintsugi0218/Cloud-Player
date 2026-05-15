// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MorphAbilityComponent.h"
#include "MyMorphAbilityInputReceiver.h"
#include "PushableActor.h"
#include "BearPushAbility.generated.h"

/**
 * 
 */
UCLASS()
class CLOUD_TEST_API UBearPushAbility : public UMorphAbilityComponent,public IMyMorphAbilityInputReceiver
{
	GENERATED_BODY()

public:

	UBearPushAbility();
	
protected:
	UPROPERTY(VisibleAnywhere)
	bool bIsPushing = false;

	UPROPERTY()
	TObjectPtr<APushableActor> CurrentPushActor;

	UPROPERTY(EditAnywhere)
	float CheckDistance = 150.f;

	UPROPERTY(EditAnywhere)
	float PushSpeed = 100.f;


public:

	virtual bool HandleAbilityInputPressed_Implementation(int32 SlotIndex) override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	virtual void OnAbilityAdded_Implementation() override;

	virtual void OnAbilityRemoved_Implementation() override;
private:

	void TryStartPush();

	void StopPush();

	APushableActor* FindPushableActor();
};
