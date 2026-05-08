// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameMode.h"
#include "MyGameInstance.h"

//void AMyGameMode::BeginPlay()
//{
//	Super::BeginPlay();
//
//	UE_LOG(LogTemp, Warning, TEXT("=== MyGameMode Running ==="));
//}
//
//
//void AMyGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
//{
//	Super::EndPlay(EndPlayReason);
//
//	UE_LOG(LogTemp, Warning, TEXT("GameMode EndPlay: %d"), (int32)EndPlayReason);
//
//    if (EndPlayReason == EEndPlayReason::Quit ||
//        EndPlayReason == EEndPlayReason::EndPlayInEditor)
//    {
//        if (UGameInstance* GI = GetGameInstance())
//        {
//            if (UMyGameInstance* MyGI = Cast<UMyGameInstance>(GI))
//            {
//                MyGI->SaveGame();
//            }
//        }
//    }
//}
