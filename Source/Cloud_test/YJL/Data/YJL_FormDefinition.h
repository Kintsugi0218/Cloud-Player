// YJL_FormDefinition.h
// 形态定义（对应 Godot 项目的 FormDefinition Resource）。
// 用纯 USTRUCT 而非 UDataAsset，避免你需要进编辑器右键创建资产。
// 实际形态实例由 C++ 代码硬编码在 UYJL_FormManagerComponent::InitDefaultForms() 里。
#pragma once

#include "CoreMinimal.h"
#include "YJL_FormDefinition.generated.h"

// 形态特殊能力枚举（Space 在 bCanJump==false 时触发）
UENUM(BlueprintType)
enum class EYJLFormSpecialAction : uint8
{
    None        UMETA(DisplayName = "None"),
    GrabDrop    UMETA(DisplayName = "GrabDrop (Bear)"),
    Glide       UMETA(DisplayName = "Glide (Future)"),
    Dash        UMETA(DisplayName = "Dash (Future)"),
};

USTRUCT(BlueprintType)
struct FYJLFormDefinition
{
    GENERATED_BODY()

    // 唯一标识（如 "default" / "bear"）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Form")
    FName Id = NAME_None;

    // UI 显示名（中文）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Form")
    FString DisplayName = TEXT("默认");

    // UI 色块颜色（变身蓄力条上面的小方块）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Form")
    FLinearColor UIColor = FLinearColor(0.3f, 0.6f, 1.0f, 1.0f);

    // ===== 移动参数（Godot 单位是米；UE 单位是厘米，所以这里乘 100）=====
    // 最大行走速度（cm/s）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Movement")
    float Speed = 600.0f;

    // 加速度（cm/s^2）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Movement")
    float Acceleration = 4000.0f;

    // 摩擦（停止减速，cm/s^2）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Movement")
    float BrakingDeceleration = 5000.0f;

    // ===== 跳跃参数 =====
    // 是否可以跳跃。false 时按 Space 不跳，而是触发 SpecialAction（如小熊抓物）。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    bool bCanJump = true;

    // 期望跳跃高度（cm）。Godot 默认 2m → 200cm
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    float JumpHeight = 200.0f;

    // 到达最高点的时间（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    float TimeToPeak = 0.4f;

    // 从最高点下落到地面的时间（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Jump")
    float TimeToDescent = 0.35f;

    // ===== 特殊能力 =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Special")
    EYJLFormSpecialAction SpecialAction = EYJLFormSpecialAction::None;

    // ===== 派生计算 =====
    // 起跳初速度 v = 2 * h / t_peak
    FORCEINLINE float GetJumpZVelocity() const
    {
        return TimeToPeak > KINDA_SMALL_NUMBER ? (2.0f * JumpHeight) / TimeToPeak : 0.0f;
    }
    // 上升期重力（正值，单位 cm/s^2）g_up = 2h / t_peak^2
    FORCEINLINE float GetJumpGravity() const
    {
        return TimeToPeak > KINDA_SMALL_NUMBER ? (2.0f * JumpHeight) / (TimeToPeak * TimeToPeak) : 980.0f;
    }
    // 下落期重力（正值）g_fall = 2h / t_descent^2
    FORCEINLINE float GetFallGravity() const
    {
        return TimeToDescent > KINDA_SMALL_NUMBER ? (2.0f * JumpHeight) / (TimeToDescent * TimeToDescent) : 980.0f;
    }
};
