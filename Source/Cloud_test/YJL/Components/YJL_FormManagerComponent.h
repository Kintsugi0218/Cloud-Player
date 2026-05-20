// YJL_FormManagerComponent.h
// 形态管理器（挂在 AYJL_PlayerCharacter 上）。
// 对应 Godot form_manager.gd。
//
// 职责：
//   1. 维护"所有已注册形态"（代码硬编码 default + bear）
//   2. 维护"已解锁形态列表"（从 UYJL_GameStateSubsystem 拉取）
//   3. 当前激活形态切换：把参数推给 Player（ApplyFormStats）
//   4. Q/E 选 pending → 蓄力 1s → 真正变身
//   5. 在 bCanJump=false 的形态下，Player 按 Space 调用 TrySpecialAction()
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Data/YJL_FormDefinition.h"
#include "YJL_FormManagerComponent.generated.h"

class AYJL_PlayerCharacter;
class UYJL_CarryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLOnFormChanged, const FYJLFormDefinition&, NewForm);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYJLOnFormChargeChanged, FName, PendingId, float, ChargeAlpha);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYJLOnFormPanelHidden);

UCLASS(ClassGroup=(YJL), meta=(BlueprintSpawnableComponent))
class CLOUD_TEST_API UYJL_FormManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UYJL_FormManagerComponent();

    // ===== 配置 =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Form")
    float ChargeTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Form")
    float IdleDismissTime = 0.6f;

    // ===== 信号 =====
    UPROPERTY(BlueprintAssignable, Category = "YJL|Form")
    FYJLOnFormChanged OnFormChanged;

    UPROPERTY(BlueprintAssignable, Category = "YJL|Form")
    FYJLOnFormChargeChanged OnFormChargeChanged;

    UPROPERTY(BlueprintAssignable, Category = "YJL|Form")
    FYJLOnFormPanelHidden OnFormPanelHidden;

    // ===== 状态查询 =====
    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    const FYJLFormDefinition& GetCurrentForm() const { return CurrentForm; }

    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    bool IsTransforming() const { return bIsTransforming; }

    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    void PickPrev();

    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    void PickNext();

    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    bool FindFormDefinition(FName FormId, FYJLFormDefinition& OutForm) const;

    // 切到 default 形态（如果已注册）；外部 API
    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    void ApplyFormById(FName FormId);

    // 形态特殊能力（Player 在 bCanJump=false 时按 Space 调用）
    UFUNCTION(BlueprintCallable, Category = "YJL|Form")
    void TrySpecialAction();

    // 强制刷新已解锁列表（GameState 解锁信号触发时）
    void RefreshUnlockedFormsFromGameState();

    // 接收 GameState 的 OnFormUnlocked 广播
    UFUNCTION()
    void HandleFormUnlocked(FName FormId);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ===== 内部数据 =====
    // 全部形态注册表（id → FormDefinition）
    UPROPERTY()
    TMap<FName, FYJLFormDefinition> Registry;

    // 已解锁形态 id 顺序（决定 Q/E 切换的轮换顺序）
    UPROPERTY()
    TArray<FName> UnlockedOrder;

    // 当前激活形态
    UPROPERTY()
    FYJLFormDefinition CurrentForm;

    // pending 形态（Q/E 选中但还在蓄力）
    UPROPERTY()
    FName PendingId = NAME_None;

    float ChargeTimer = 0.0f;
    float IdleTimer = 0.0f;
    bool bPanelVisible = false;
    bool bIsTransforming = false;

    // 初始化硬编码形态
    void InitDefaultForms();

    // 切换 pending 形态（Q/E 公用）
    void Step(int32 Direction);

    // 应用形态（真正变身）
    void ApplyForm(const FYJLFormDefinition& Form);

    // 找到 PendingId 在 UnlockedOrder 中的下标；找不到返回 -1
    int32 IndexOf(FName Id) const;

    // 引用：Player + Carry
    UPROPERTY() TObjectPtr<AYJL_PlayerCharacter> OwnerPlayer;
    UPROPERTY() TObjectPtr<UYJL_CarryComponent> CarryRef;
};
