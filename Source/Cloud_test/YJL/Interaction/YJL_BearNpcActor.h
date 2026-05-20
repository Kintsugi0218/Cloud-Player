// YJL_BearNpcActor.h
// 小熊 NPC（对应 Godot bear_npc.gd）。
// 对话完成后解锁小熊形态。
#pragma once

#include "CoreMinimal.h"
#include "YJL_NpcActor.h"
#include "YJL_BearNpcActor.generated.h"

UCLASS()
class CLOUD_TEST_API AYJL_BearNpcActor : public AYJL_NpcActor
{
    GENERATED_BODY()

public:
    AYJL_BearNpcActor();

    // 未解锁时台词
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|BearNPC")
    TArray<FString> LinesFirst;

    // 已解锁后台词
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|BearNPC")
    TArray<FString> LinesAfter;

protected:
    virtual void StartDialogue() override;

    UFUNCTION()
    void OnDialogueCompleteUnlockBear();
};
