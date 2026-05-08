// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

class USaveManager;
class UMySaveGame;
UCLASS()
class CLOUD_TEST_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void SaveAndQuit() ;

	// 新游戏
	UFUNCTION(BlueprintCallable)
	void NewGame();

	// 保存 / 读取
	UFUNCTION(BlueprintCallable)
	void SaveGame();

	UFUNCTION(BlueprintCallable)
	void LoadGame();

	USaveManager* GetSaveManager() const { return SaveManager; }

private:

	UPROPERTY()
	USaveManager* SaveManager;

	FString SlotName = TEXT("GameData_Slot1");
};
