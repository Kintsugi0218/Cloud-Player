// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveManager.h"
#include "Kismet/GameplayStatics.h"
#include "MySaveGame.h"
#include "ISaveManager.h"



void USaveManager::SaveGame(UWorld* World, UMySaveGame* GameData)
{
    UE_LOG(LogTemp, Warning, TEXT("SaveGame Called"));
    if (!World || !GameData) return;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsWithInterface(World, UISaveManager::StaticClass(), Actors); // 找到所有实现了ISaveManager的接口


    for (AActor* Actor : Actors)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *Actor->GetName());
        IISaveManager::Execute_SaveData(Actor, GameData);
    }

    bool bSuccess = UGameplayStatics::SaveGameToSlot(GameData, TEXT("GameData_Slot1"), 0);

    UE_LOG(LogTemp, Warning, TEXT("Save Result: %d"), bSuccess);
}

void USaveManager::LoadGame(UWorld* World, UMySaveGame* GameData)
{
    UE_LOG(LogTemp, Warning, TEXT("LoadGame Called"));

    if (!World || !GameData) return;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsWithInterface(World, UISaveManager::StaticClass(), Actors);

    UE_LOG(LogTemp, Warning, TEXT("Actors Num: %d"), Actors.Num());

    for (AActor* Actor : Actors)
    {
        IISaveManager::Execute_LoadData(Actor, GameData);
    }
}
