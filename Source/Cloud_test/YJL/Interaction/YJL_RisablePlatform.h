// YJL_RisablePlatform.h
// 可升降平台 —— 对应 Godot forms/risable_platform.gd。
//
// 行为：
//   - 通过关联的 AYJL_PressurePlate 的 OnActivated / OnDeactivated 信号驱动；
//     也可在外部直接 Blueprint 调 RaiseUp() / LowerDown()。
//   - Tick 中把根 Mesh 的 Z 朝 TargetZ 插值（玩家站在上面会被一起带起）。
//
// 用法（编辑器里）：
//   1. 把 BP_YJL_RisablePlatform 拖进关卡
//   2. Details → YJL|Platform → LinkedPlate 拖一个 AYJL_PressurePlate 实例
//   3. 调 RaisedZ / LoweredZRelative 决定升 / 降的高度（cm，相对生成位置）
//
// 注意：
//   - 根是 UStaticMeshComponent + BlockAll，玩家能站在上面
//   - 玩家被带起的方式：UE Character 的 MovementComponent 会跟随"Base"
//     （StaticMesh 设为 SetMobility(Movable) 后会有效）

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJL_RisablePlatform.generated.h"

class UStaticMeshComponent;
class AYJL_PressurePlate;

UCLASS()
class CLOUD_TEST_API AYJL_RisablePlatform : public AActor
{
    GENERATED_BODY()

public:
    AYJL_RisablePlatform();

    // ===== 组件 =====
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;

    // ===== 参数 =====
    // 关联的压力板（拖一个 AYJL_PressurePlate 实例，不填则只能通过 Blueprint/外部触发）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform")
    TObjectPtr<AYJL_PressurePlate> LinkedPlate;

    // 升起后相对初始位置的 Z 偏移（cm，正值=往上）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform")
    float RaisedZRelative = 200.0f;

    // 下降态相对初始位置的 Z 偏移（cm，0=就是 Actor 在 Editor 里摆的位置）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform")
    float LoweredZRelative = 0.0f;

    // 升 / 降的插值速度（每秒比例 ≈ 1 - exp(-Speed*dt)）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform", meta=(ClampMin="0.1"))
    float TransitionInterpSpeed = 3.0f;

    // 解除触发时是否自动下降（false = 一次性升起就不再回去）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform")
    bool bLowerOnDeactivate = true;

    // 屏字调试
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Platform")
    bool bScreenLog = true;

    // ===== 控制 =====
    UFUNCTION(BlueprintCallable, Category = "YJL|Platform")
    void RaiseUp();

    UFUNCTION(BlueprintCallable, Category = "YJL|Platform")
    void LowerDown();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "YJL|Platform")
    bool IsRaised() const { return bIsRaised; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void HandlePlateActivated(AYJL_PressurePlate* Plate);

    UFUNCTION()
    void HandlePlateDeactivated(AYJL_PressurePlate* Plate);

    bool bIsRaised = false;
    float InitialZ = 0.0f;   // BeginPlay 时记录的世界 Z
    float TargetZ = 0.0f;    // 当前 Tick 目标
};
