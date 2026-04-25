// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyNpc.generated.h"

UCLASS()
class CLOUD_TEST_API AMyNpc : public ACharacter
{
	GENERATED_BODY()

public:
	AMyNpc();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
