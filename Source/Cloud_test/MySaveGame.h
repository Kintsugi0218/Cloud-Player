// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MyCameraRigActor.h" 
#include "MySaveGame.generated.h"

class AMyPlayerCharacter;
class UMyMorphDataAsset;


UCLASS()
class CLOUD_TEST_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY() // 需要保存的变量都要管理
	FVector PlayerLocation;

	UPROPERTY()
	TArray<UMyMorphDataAsset*> UnlockedMorphs;

	UPROPERTY()
	UMyMorphDataAsset* LastMorph;

	UPROPERTY()
	FVector CameraRigPosition;

	UPROPERTY()
	FMyCameraProfile VolumeProfile;


	UPROPERTY()
	TMap<FString, FVector> NPCPositions;
	UPROPERTY()
	TMap<FString, bool> NPCAbility;
};
