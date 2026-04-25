#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyMorphDataAsset.generated.h"

class USkeletalMesh;
class UMaterialInterface;
class UActorComponent;
class UInputMappingContext;

/**
 * @brief 形态数据资产，用于存储每个角色形态的配置。
 */
UCLASS(BlueprintType)
class CLOUD_TEST_API UMyMorphDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // --- 基础移动属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float MaxWalkSpeed = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float AirControl = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float GravityScale = 0.35f;

    // --- 跳跃属性 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
    float MaxJumpHeight = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
    float JumpSpeed = 300.0f;

    // --- 外观 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UMaterialInterface> Material;

    // --- 特殊能力组件 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UActorComponent>> AbilityComponents;

    // --- 形态专属输入映射 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> MorphMappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    int32 MorphMappingPriority = 1;

    // --- 解锁标签 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    FName UnlockTag = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};