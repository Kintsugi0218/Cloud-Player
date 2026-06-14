#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyMorphDataAsset.generated.h"

class USkeletalMesh;
class UMaterialInterface;
class UActorComponent;
class UInputMappingContext;
class UMorphAbilityComponent;



USTRUCT(BlueprintType)
struct FMorphCloudSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector VolumeSize = FVector(1700.f, 2000.f, 1400.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector LocalPivot = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ResolutionMaxAxis = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float SpawnRate = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float LifetimeMin = 13.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float LifetimeMax = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float RadiusMin = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float RadiusMax = 270.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float DensityMin = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float DensityMax = 1.0f;
};
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
    float MaxWalkSpeed = 450.0f; // 最大行走速度

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
    float AirControl = 0.6f; // 空中可控程度

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

    // --- ABP ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    TSoftClassPtr<UAnimInstance> AnimBPClass;

    UPROPERTY(EditAnywhere)
    FVector MeshLocation;

    UPROPERTY(EditAnywhere)
    FRotator MeshRotation;

    UPROPERTY(EditAnywhere)
    FVector MeshScale;

    UPROPERTY(EditAnywhere)
    float CapsuleHalfHeight;

    UPROPERTY(EditAnywhere)
    float CapsuleRadius;

    UPROPERTY(EditAnywhere)
    float TargetArmLength;

    // --- 特殊能力组件 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UMorphAbilityComponent>> AbilityComponents;

    // --- 形态专属输入映射 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> MorphMappingContext = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    int32 MorphMappingPriority = 1;

    // --- 解锁标签 ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unlock")
    FName UnlockTag = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Description")
    FText Description;


    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cloud")
    FMorphCloudSettings CloudSettings;
};