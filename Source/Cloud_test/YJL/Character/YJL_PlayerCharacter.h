// YJL_PlayerCharacter.h
// 全新独立的玩家角色 —— 不继承也不修改队友的 AMyPlayerCharacter。
// 复刻 Godot player.gd：
//   - WASD 相对相机水平移动 + 加速/摩擦
//   - 鼠标控制相机 Yaw/Pitch（Pitch ±80°）
//   - Space 跳跃（独立重力 + coyote time + jump buffer）
//   - Q/E 切换形态（蓄力 1s）
//   - 形态 bCanJump=false 时 Space 触发 SpecialAction
//   - 对话激活时冻结输入
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "YJL_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UYJL_FormManagerComponent;
class UYJL_CarryComponent;
class UWidgetComponent;

UCLASS()
class CLOUD_TEST_API AYJL_PlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AYJL_PlayerCharacter();

    // ===== 组件 =====
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UYJL_FormManagerComponent> FormManager;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UYJL_CarryComponent> CarryComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UWidgetComponent> FormSelectorWidgetComp;

    // 角色可见身体（StaticMesh 占位，默认 Cylinder；可在 BP 里把 StaticMesh 字段换成你想要的）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UStaticMeshComponent> VisualBody;

    // ===== 输入参数（Godot 默认值的直译）=====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Camera")
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Camera")
    float PitchMinDegrees = -80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Camera")
    float PitchMaxDegrees = 80.0f;

    // 鼠标 Y 轴反转（在 Details 里勾选即可切换，不用改代码）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Camera")
    bool bInvertMouseY = false;

    // 跳跃辅助
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    float CoyoteTimeDuration = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    float JumpBufferDuration = 0.15f;

    // ===== 输入资产（在 BP_YJL_Player 的 Class Defaults 里挂 Editor 创建好的 .uasset）=====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input")
    TObjectPtr<UInputMappingContext> DefaultIMC;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_Move;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_Look;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_Jump;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_Interact;       // F：NPC 对话 / 抓取
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_FormPrev;       // Q
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Input") TObjectPtr<UInputAction> IA_FormNext;       // E

    // ===== 当前 PlayerController 引用（便捷）=====
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> CachedPC;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;

    // 强制把鼠标锁进游戏视口（Possess 后 / BeginPlay 后 / 必要时随时调）
    void EnsureGameInputMode();

    // ===== 输入回调 =====
    void OnMove(const FInputActionValue& Value);
    void OnLook(const FInputActionValue& Value);
    void OnJumpPressed(const FInputActionValue& Value);
    void OnInteractPressed(const FInputActionValue& Value);
    void OnFormPrevPressed(const FInputActionValue& Value);
    void OnFormNextPressed(const FInputActionValue& Value);

    // 给 PlayerController 装上 IMC（EnhancedInput 子系统）
    void RegisterIMC();

    // 对话进行中？（从 GameInstance 的 DialogueSubsystem 查询）
    bool IsDialogueActive() const;

    // ===== 跳跃辅助计时 =====
    float CoyoteTimer = 0.0f;
    float JumpBufferTimer = 0.0f;
    bool bWantsToJump = false;

    // 应用形态参数（FormManager 切换形态时调用）
public:
    void ApplyFormStats(const struct FYJLFormDefinition& Form);

    // 暴露给抓取组件用的便捷方法：玩家前方点
    FVector GetFrontPoint(float DistanceCm) const;
};
