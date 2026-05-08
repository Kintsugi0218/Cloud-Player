// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SaveManager.generated.h"


class UMySaveGame;

UCLASS()
class CLOUD_TEST_API USaveManager : public UObject
{
	GENERATED_BODY()
	

public:

    void SaveGame(UWorld* World, UMySaveGame* GameData);
    void LoadGame(UWorld* World, UMySaveGame* GameData);


};
