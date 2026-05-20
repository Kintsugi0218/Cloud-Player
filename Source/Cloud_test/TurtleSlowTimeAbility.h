// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MorphAbilityComponent.h"
#include "TurtleSlowTimeAbility.generated.h"

/**
 * 
 */
UCLASS()
class CLOUD_TEST_API UTurtleSlowTimeAbility : public UMorphAbilityComponent
{
	GENERATED_BODY()
public:
    virtual void OnAbilityAdded_Implementation() override;

    virtual void OnAbilityRemoved_Implementation() override;

protected:

    UPROPERTY(EditAnywhere, Category = "Slow Time")
    float GlobalTimeScale = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Slow Time")
    float PlayerTimeScale = 0.7f;
	
};
