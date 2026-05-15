// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MorphAbilityComponent.generated.h"

class AMyPlayerCharacter;

UCLASS(Abstract,ClassGroup = (Morph), Blueprintable, BlueprintType,meta = (BlueprintSpawnableComponent))
class CLOUD_TEST_API UMorphAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMorphAbilityComponent();

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnAbilityAdded(); // 不能重写,只是用来调用

	virtual void OnAbilityAdded_Implementation(); // 需要重写的是此函数

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	void OnAbilityRemoved();

	virtual void OnAbilityRemoved_Implementation();


	UFUNCTION(BlueprintCallable)
	virtual void SetAbilityEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable)
	bool IsAbilityEnabled() const { return bAbilityEnabled; };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ability")
	FName AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	bool bPassiveAbility = false;


protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	bool bAbilityEnabled = true;

	UPROPERTY()
	TObjectPtr<AMyPlayerCharacter> CachedPlayer;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
