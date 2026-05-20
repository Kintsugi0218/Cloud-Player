// YJL_PressurePlate.h
// 压力板机关 —— 对应 Godot forms/pressure_plate.gd。
//
// 场景结构（在 BP_YJL_PressurePlate 里搭好，玩家在场景里直接拖 BP 即可）：
//   AYJL_PressurePlate (root = BaseBody UBoxComponent，让玩家/物体能踩)
//     ├─ VisualMesh   (UStaticMeshComponent；被压下时下沉)
//     └─ TriggerVolume(UBoxComponent，QueryOnly + OverlapAll，板面上方薄层)
//
// 工作流程：
//   1. TriggerVolume 用 BeginOverlap/EndOverlap 维护"上面有哪些 Actor"
//   2. 任意进出 → 重新结算总重量
//   3. 玩家（AYJL_PlayerCharacter / ACharacter）→ PlayerWeight
//      带 UYJL_CarryableComponent 的 → 该组件的 Weight 字段
//      其他 → 不计
//   4. 总重达到 ActivationWeight → 激活，发 OnActivated 多播
//      总重低于 DeactivationWeight 且未 latch → 失活，发 OnDeactivated
//   5. Tick 里把 VisualMesh.Z 朝目标 lerp（按下=低位 / 弹起=高位）

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJL_PressurePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

// 蓝图可绑定的多播：参数 = 触发该状态的 PressurePlate 自身
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLPressurePlateEvent, AYJL_PressurePlate*, Plate);

UCLASS()
class CLOUD_TEST_API AYJL_PressurePlate : public AActor
{
    GENERATED_BODY()

public:
    AYJL_PressurePlate();

    // ===== 组件 =====
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UBoxComponent> BaseBody;       // 物理体（让玩家踩）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UStaticMeshComponent> VisualMesh; // 视觉（按下时下沉）

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YJL|Components")
    TObjectPtr<UBoxComponent> TriggerVolume;  // 触发器（板面上方薄层）

    // ===== 参数 =====
    // 触发所需的最小总重量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate", meta=(ClampMin="0.0"))
    float ActivationWeight = 2.0f;

    // 退出触发所需的总重量阈值（应略小于 ActivationWeight，避免抖动）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate", meta=(ClampMin="0.0"))
    float DeactivationWeight = 1.0f;

    // 玩家算作多少重量（ACharacter 没有 Mass，所以用这个值近似）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate", meta=(ClampMin="0.0"))
    float PlayerWeight = 5.0f;

    // 触发后是否锁定（即使物体离开也保持触发状态）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate")
    bool bActivationLatched = false;

    // 板子被压下后的视觉下沉量（cm）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate", meta=(ClampMin="0.0"))
    float PressDepth = 8.0f;

    // 视觉下沉 / 弹起的插值速度（每秒接近目标的比例 ≈ 1 - exp(-Speed*dt)）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate", meta=(ClampMin="0.1"))
    float PressInterpSpeed = 12.0f;

    // 屏字调试输出
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Plate")
    bool bScreenLog = true;

    // ===== 信号 =====
    UPROPERTY(BlueprintAssignable, Category = "YJL|Plate")
    FYJLPressurePlateEvent OnActivated;

    UPROPERTY(BlueprintAssignable, Category = "YJL|Plate")
    FYJLPressurePlateEvent OnDeactivated;

    // ===== 查询 =====
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "YJL|Plate")
    bool IsActive() const { return bIsActive; }

    // 手动复位（关卡脚本可用）
    UFUNCTION(BlueprintCallable, Category = "YJL|Plate")
    void ResetPlate();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // Overlap 回调
    UFUNCTION()
    void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

    UFUNCTION()
    void HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // 重新结算
    void Evaluate();

    // 计算单个 Actor 在压力板上的重量贡献
    float WeightOf(AActor* Other) const;

    // 切换激活状态
    void SetActive(bool bNewActive);

    // 当前是否激活
    bool bIsActive = false;

    // VisualMesh 的"静止 Z"（出生时记录，按下/弹起都基于此）
    float VisualRestZ = 0.0f;

    // VisualMesh 当前目标 Z
    float VisualTargetZ = 0.0f;
};
