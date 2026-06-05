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

    GameData = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass()));

   

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

    if (!GameData) return;

    GameData->Currentkey.Add(TEXT("MoveForward"), EKeys::W);
    GameData->Currentkey.Add(TEXT("MoveBackward"), EKeys::S);
    GameData->Currentkey.Add(TEXT("MoveLeft"), EKeys::A);
    GameData->Currentkey.Add(TEXT("MoveRight"), EKeys::D);
    GameData->Currentkey.Add(TEXT("Jump"), EKeys::SpaceBar);
    GameData->Currentkey.Add(TEXT("Ability"), EKeys::G);
    GameData->Currentkey.Add(TEXT("Menu"), EKeys::M);
    GameData->Currentkey.Add(TEXT("Interact"), EKeys::F);
    GameData->Currentkey.Add(TEXT("MorphNext"), EKeys::E);
    GameData->Currentkey.Add(TEXT("MorphPrev"), EKeys::Q);

    UWorld* World = GetWorld();

    // 初始化必要默认值（只写必要的）
    GameData->PlayerLocation = FVector(0, 600, 300);

    // 应用初始数据
    SaveManager->LoadGame(World,GameData);

    // 立即保存一份
    SaveManager->SaveGame(World,GameData);


}

void UMyGameInstance::SaveGame()
{
    if (!SaveManager) return;

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
   
    GameData = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("GameData_Slot1"), 0));

    UWorld* World = GetWorld();

    SaveManager->LoadGame(World,GameData);

}
