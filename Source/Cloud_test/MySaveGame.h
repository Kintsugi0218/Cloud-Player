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

	UPROPERTY(SaveGame) // 需要保存的变量都要管理
	FVector PlayerLocation;

	UPROPERTY(SaveGame)
	TArray<UMyMorphDataAsset*> UnlockedMorphs;

	UPROPERTY(SaveGame)
	UMyMorphDataAsset* LastMorph;

	UPROPERTY(SaveGame)
	FVector CameraRigPosition;

	UPROPERTY(SaveGame)
	FMyCameraProfile VolumeProfile;


	UPROPERTY(SaveGame)
	TMap<FString, FVector> NPCPositions;

	UPROPERTY(SaveGame,EditAnywhere,BlueprintReadWrite)
	TMap<FString, int> NPCNextPositionIndex;

	UPROPERTY(SaveGame)
	TMap<FString, bool> NPCAbility;

	UPROPERTY(SaveGame)
	TMap<FString, FVector> PushActorPositions;

	UPROPERTY(SaveGame)
	TMap<FString, bool> bPushActorCanBePushed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TMap<FName, FKey> Currentkey;

};