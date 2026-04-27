// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

/**
 * 
 */
UCLASS()
class CLOUD_TEST_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:

    UFUNCTION(BlueprintImplementableEvent)
    void InitDialogue(const TArray<FString>& Lines);

    UFUNCTION(BlueprintImplementableEvent)
    void SetPlayerRef(class AMyPlayerCharacter* Player);
};
