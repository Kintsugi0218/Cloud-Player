// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "SaveManager.h"
#include "MySaveGame.h"
#include "Kismet/GameplayStatics.h"

void UMyGameInstance::Init()
{
    Super::Init();

    SaveManager = NewObject<USaveManager>(this); // 创建归属于GameInstance的SaveManager（生命周期跟随GameInstance）

    // LoadGame(); // 在这里会导致LoadGame调用过早，可以放在player的BeginPlay中
}

void UMyGameInstance::SaveAndQuit()
{
    // 游戏退出时自动保存
    SaveGame();

    UKismetSystemLibrary::QuitGame(
        this,
        nullptr,
        EQuitPreference::Quit,
        false
    );
}

void UMyGameInstance::NewGame()
{
    if (!SaveManager) return;

    UMySaveGame* GameData = Cast<UMySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())
    );

    if (!GameData) return;

    UWorld* World = GetWorld();

    // 初始化必要默认值（只写必要的）
    GameData->PlayerLocation = FVector(-200, 0, 1142);

    // 应用初始数据
    SaveManager->LoadGame(World,GameData);

    // 立即保存一份
    SaveManager->SaveGame(World,GameData);


}

void UMyGameInstance::SaveGame()
{
    if (!SaveManager) return;

    UMySaveGame* GameData = Cast<UMySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())
    );

    if (!GameData) return;

    UWorld* World = GetWorld();

    SaveManager->SaveGame(World,GameData);

    UGameplayStatics::SaveGameToSlot(GameData, SlotName, 0);
}

void UMyGameInstance::LoadGame()
{

    if (!SaveManager) return;

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        // 没有存档 → 新游戏
        NewGame();
        return;
    }
   
    UMySaveGame* GameData = Cast<UMySaveGame>(
        UGameplayStatics::LoadGameFromSlot(TEXT("GameData_Slot1"), 0)
    );

    UWorld* World = GetWorld();

    SaveManager->LoadGame(World,GameData);

}
