// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "Blueprint/UserWidget.h"

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("ToggleMenu", IE_Pressed, this, &AMyPlayerController::ToggleMenu);
}


void AMyPlayerController::ToggleMenu()
{
    UE_LOG(LogTemp, Warning, TEXT("Toogle menu Called"));

    if (!MainMenuClass) return;
    UE_LOG(LogTemp, Warning, TEXT("have MainMenuClass"));
    if (!bMenuOpen)
    {
        // 打开 UI
        MainMenu = CreateWidget<UUserWidget>(this, MainMenuClass);
        UE_LOG(LogTemp, Warning, TEXT("create Mainmenu"));
        if (MainMenu)
        {
            MainMenu->AddToViewport();

            // 切UI模式
            FInputModeGameAndUI InputMode;
            InputMode.SetWidgetToFocus(MainMenu->TakeWidget());
            SetInputMode(InputMode);

            bShowMouseCursor = true;
        }

        bMenuOpen = true;
    }
    else
    {
        // 关闭 UI
        if (MainMenu)
        {
            MainMenu->RemoveFromParent();
            MainMenu = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("remove mainmenu"));
        }

        // 切回游戏模式
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);

        bShowMouseCursor = false;

        bMenuOpen = false;
    }
}