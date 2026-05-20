// YJL_GodotTscnLoader.h
// Godot .tscn 文件运行时/编辑时加载器 —— 把 Godot 场景里的白模（CSG / 简单 Mesh）
// 还原成 UE 的 StaticMeshComponent / Light 等。
//
// 用法：
//   1. 把 AYJL_GodotLevelHost 拖到关卡里
//   2. 在 Details 面板填 .tscn 文件绝对路径
//   3. 改任何属性 → OnConstruction 触发重建（不需要 Play）
//
// 限制：
//   - 只还原白模（CSG / MeshInstance3D / DirectionalLight3D / OmniLight3D）
//   - 跳过 instance/Script/Label3D/Collision 等逻辑节点（屏字提示）
//   - 坐标系：Godot 右手 Y-up，UE 左手 Z-up，1m = 100cm
//
// 文件归属：YJL 子模块（与队友代码物理隔离）。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YJL_GodotTscnLoader.generated.h"

class USceneComponent;
class UMaterialInterface;
class UStaticMesh;

UCLASS()
class CLOUD_TEST_API AYJL_GodotLevelHost : public AActor
{
    GENERATED_BODY()

public:
    AYJL_GodotLevelHost();

    // ===== 配置（Details 面板可编辑）=====

    // .tscn 文件路径（支持项目内相对路径如 "Content/YJL/GodotScene/main_level.tscn"，或绝对路径）
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    FString TscnFilePath = TEXT("Content/YJL/GodotScene/main_level.tscn");

    // Godot 1 单位 = UE UnitScale 厘米（默认 100，即 1m=100cm）
    UPROPERTY(EditAnywhere, Category = "YJL|Import", meta=(ClampMin="1.0"))
    float UnitScale = 100.0f;

    // 找不到具名材质时用的回退材质（建议挂 /Engine/BasicShapes/BasicShapeMaterial）
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    TObjectPtr<UMaterialInterface> FallbackMaterial;

    // 材质名（key 形如 "mat_jump_platform"）→ UMat
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    TMap<FString, TObjectPtr<UMaterialInterface>> MaterialMap;

    // 勾选/取消 触发一次重建（不需要 Play）
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    bool bRebuildNow = false;

    // 是否给所有几何体打开碰撞（默认 true，地形能走）
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    bool bEnableCollision = true;

    // 是否把屏字提示打到屏幕（解析过程信息）
    UPROPERTY(EditAnywhere, Category = "YJL|Import")
    bool bScreenLog = true;

    // ===== Editor 工具按钮（在 Details 顶部 / 底部点击即可触发）=====

    // 暴力清空 Host 的所有生成几何（含累加残留），保留 RootComponent 和 GeneratedRoot
    UFUNCTION(CallInEditor, Category = "YJL|Tools")
    void ForceClearAll();

    // 先清空再重建（最稳妥的"重置"操作）
    UFUNCTION(CallInEditor, Category = "YJL|Tools")
    void ForceRebuild();

    // 诊断：在屏幕和 Output Log 打印当前 Host 持有多少 SceneComponent
    UFUNCTION(CallInEditor, Category = "YJL|Tools")
    void Diagnose();

    // 离散生成独立 Actor：将 .tscn 中的各个建模一键生成为场景中完全独立的 AStaticMeshActor 与光源 Actor
    UFUNCTION(CallInEditor, Category = "YJL|Tools")
    void SpawnAsIndependentActors();

    // ===== Lifecycle =====
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    // 生成的所有 component 都挂在这下面，方便整体清理
    UPROPERTY()
    TObjectPtr<USceneComponent> GeneratedRoot;

    // 清空之前生成的所有子组件（递归销毁所有后代 + 兜底清理孤儿）
    void ClearGeneratedChildren();

    // 主入口：读文件 + 解析 + 生成
    void BuildFromTscn();

    // 智能路径解析：如果绝对路径不存在，自动回退查找 Content/YJL/GodotScene/
    FString ResolveTscnFilePath() const;
};
