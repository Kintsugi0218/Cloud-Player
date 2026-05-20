// YJL_DialogueSubsystem.h
// 全局对话系统（对应 Godot dialogue_system.gd autoload）。
// 使用 UGameInstanceSubsystem 自动随 GameInstance 生命周期存在 → 不需要在场景里放任何东西。
//
// 职责：
//   1. 管理对话流（多句台词、当前句）
//   2. 提供 IsActive() 让 Player 判断是否冻结输入
//   3. Advance() 由 Player 在 F 键按下时调用
//   4. 通过 GEngine->AddOnScreenDebugMessage 直接把当前台词渲染到屏幕（纯代码，无 UMG）
//
// 另外：暴露 BroadcastInteractPressed() 让 NPC 监听玩家"按了 F" 事件（避免每个 NPC 自己抓 PlayerController）。
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YJL_DialogueSubsystem.generated.h"

class AYJL_PlayerCharacter;

USTRUCT(BlueprintType)
struct FYJLDialogueLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|Dialogue")
    FString Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|Dialogue")
    FString Text;
};

DECLARE_DYNAMIC_DELEGATE(FYJLDialogueFinishedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYJLDialogueStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYJLDialogueFinishedMC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLInteractPressed, AYJL_PlayerCharacter*, Player);

UCLASS()
class CLOUD_TEST_API UYJL_DialogueSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="YJL|Dialogue")
    bool IsActive() const { return bIsActive; }

    // 开始对话；OnFinished 可空
    UFUNCTION(BlueprintCallable, Category="YJL|Dialogue")
    void StartDialogue(const TArray<FYJLDialogueLine>& Lines, const FYJLDialogueFinishedDelegate& OnFinished);

    // 推进一句（F/Space）
    UFUNCTION(BlueprintCallable, Category="YJL|Dialogue")
    void Advance();

    // Player 按 F（非对话状态）→ 广播给所有 NPC
    void BroadcastInteractPressed(AYJL_PlayerCharacter* Player);

    // ===== 信号 =====
    UPROPERTY(BlueprintAssignable, Category="YJL|Dialogue")
    FYJLDialogueStarted DialogueStarted;

    UPROPERTY(BlueprintAssignable, Category="YJL|Dialogue")
    FYJLDialogueFinishedMC DialogueFinished;

    // NPC 订阅这个事件
    UPROPERTY(BlueprintAssignable, Category="YJL|Interaction")
    FYJLInteractPressed InteractPressed;

private:
    bool bIsActive = false;
    int32 LineIndex = 0;
    TArray<FYJLDialogueLine> Lines;
    FYJLDialogueFinishedDelegate OnFinishedCb;

    // 调试输出当前台词到屏幕
    void RenderCurrentLine();
    void EndDialogue();

    FTimerHandle RenderTimer;
    // 持续把当前台词刷新到屏幕（AddOnScreenDebugMessage 是临时显示，需要每帧重渲染）
    void TickRender();
};
