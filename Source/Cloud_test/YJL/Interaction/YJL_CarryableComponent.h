// YJL_CarryableComponent.h
// 挂在"可被抓取的 Actor"上的标记组件（对应 Godot carryable.gd）。
// 使用方式：任何 AActor 添加此组件即可，被 CarryComponent 探测到时显示提示。
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YJL_CarryableComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup=(YJL), meta=(BlueprintSpawnableComponent))
class CLOUD_TEST_API UYJL_CarryableComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UYJL_CarryableComponent();

    // UI 上显示的名字（如 "木箱"）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carryable")
    FString DisplayName = TEXT("箱子");

    // 抓起后挂在挂载点的额外偏移
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carryable")
    FVector CarryOffset = FVector::ZeroVector;

    // 抓起后是否保持直立
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carryable")
    bool bKeepUpright = true;

    // 物体重量（压力板用于结算总重；脱离物理仿真时 UE 拿不到可靠 mass，所以这里手填）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "YJL|Carryable", meta=(ClampMin="0.0"))
    float Weight = 4.0f;

    // 获取 Owner 的根碰撞组件
    UPrimitiveComponent* GetPhysicsRoot() const;
};
