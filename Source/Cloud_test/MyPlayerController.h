// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CLOUD_TEST_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

    virtual void SetupInputComponent() override;

    // UIÀà
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> MainMenuClass;

    // UIÊµÀý
    UPROPERTY()
    UUserWidget* MainMenu;

    // ×´Ì¬
    bool bMenuOpen = false;

    void ToggleMenu();
};
