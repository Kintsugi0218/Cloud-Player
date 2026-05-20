// Fill out your copyright notice in the Description page of Project Settings.


#include "TurtleSlowTimeAbility.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerCharacter.h"


void UTurtleSlowTimeAbility::OnAbilityAdded_Implementation()
{
    Super::OnAbilityAdded_Implementation();

    UGameplayStatics::SetGlobalTimeDilation(GetWorld(),GlobalTimeScale);

    AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(GetOwner());

    if (Player)
    {
        Player->CustomTimeDilation = PlayerTimeScale / GlobalTimeScale;
    }
}

void UTurtleSlowTimeAbility::OnAbilityRemoved_Implementation()
{
    Super::OnAbilityRemoved_Implementation();

    UGameplayStatics::SetGlobalTimeDilation(GetWorld(),1.0f);

    AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(GetOwner());

    if (Player)
    {
        Player->CustomTimeDilation = 1.0f;
    }
}
