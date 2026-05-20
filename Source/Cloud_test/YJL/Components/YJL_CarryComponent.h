// YJL_CarryComponent.h
// 抓取控制器（挂在 AYJL_PlayerCharacter 上）。
// 对应 Godot carry_controller.gd。
//
// 职责：
//   - 在玩家前方维护一个 USphereComponent 探测体，检测带 UYJL_CarryableComponent 的 Actor
//   - 维护一个 USceneComponent CarrySlot 作为抓起物的挂载点
//   - 提供 TryGrabOrDrop()，由 FormManager 在 SpecialAction==GrabDrop 时调用
//   - SetActive(true/false) 控制是否启用探测（小熊形态启用）
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YJL_CarryComponent.generated.h"

class USphereComponent;
class USceneComponent;
class UYJL_CarryableComponent;
class AYJL_PlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLOnCarryHintChanged, const FString&, NewHint);

UCLASS(ClassGroup=(YJL), meta=(BlueprintSpawnableComponent))
class CLOUD_TEST_API UYJL_CarryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UYJL_CarryComponent();

    // ===== 配置 =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carry")
    float DetectRadiusCm = 160.0f;

    // 玩家局部空间偏移（前方 60cm，胸前 100cm 高）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carry")
    FVector DetectOffset = FVector(60.0f, 0.0f, 100.0f); // UE: X=前 Y=右 Z=上

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carry")
    FVector SlotOffset = FVector(70.0f, 0.0f, 100.0f);

    // ===== 信号 =====
    UPROPERTY(BlueprintAssignable, Category = "YJL|Carry")
    FYJLOnCarryHintChanged OnHintChanged;

    // ===== API =====
    UFUNCTION(BlueprintCallable, Category = "YJL|Carry")
    void SetActive_YJL(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "YJL|Carry")
    bool IsActiveYJL() const { return bActive; }

    UFUNCTION(BlueprintCallable, Category = "YJL|Carry")
    bool IsHolding() const { return Held != nullptr; }

    // Space 入口：按状态决定抓 / 放。返回 true 表示本次按键消费了。
    UFUNCTION(BlueprintCallable, Category = "YJL|Carry")
    bool TryGrabOrDrop();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ===== 运行时子组件（在 BeginPlay 里 NewObject + Attach）=====
    UPROPERTY() TObjectPtr<USphereComponent> Detector;
    UPROPERTY() TObjectPtr<USceneComponent> CarrySlot;

    // ===== 当前候选 / 手中物体 =====
    UPROPERTY() TObjectPtr<UYJL_CarryableComponent> Candidate;
    UPROPERTY() TObjectPtr<UYJL_CarryableComponent> Held;

    bool bActive = false;

    // 缓存抓起时被覆盖的物理状态（放下时恢复）
    bool bCachedSimulatePhysics = false;
    ECollisionEnabled::Type CachedCollisionEnabled = ECollisionEnabled::NoCollision;

    UPROPERTY() TObjectPtr<AYJL_PlayerCharacter> OwnerPlayer;

    UYJL_CarryableComponent* FindNearestCandidate() const;
    void PickUp(UYJL_CarryableComponent* Carryable);
    void DropHeld();
    void UpdateHint();
};
