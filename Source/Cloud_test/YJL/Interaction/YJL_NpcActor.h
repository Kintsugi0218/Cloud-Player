// YJL_NpcActor.h
// 通用 NPC（对应 Godot npc.gd / bear_npc.gd 的共同部分）。
// 玩家进入触发盒 → 显示"[F] 对话"提示；玩家按 F → 启动 DialogueSubsystem 对话。
//
// 不需要在场景里手动放：可通过控制台命令 YJL_SpawnNpc / YJL_SpawnBearNpc 运行时生成。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Dialogue/YJL_DialogueSubsystem.h"
#include "YJL_NpcActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AYJL_PlayerCharacter;

UCLASS()
class CLOUD_TEST_API AYJL_NpcActor : public AActor
{
    GENERATED_BODY()

public:
    AYJL_NpcActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YJL|NPC")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YJL|NPC")
    TObjectPtr<UBoxComponent> Trigger;

    // NPC 名字（屏幕显示用）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|NPC")
    FString NpcName = TEXT("村民");

    // 对话台词（每行一句；硬编码到属性里方便子类覆盖）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YJL|NPC")
    TArray<FString> DialogLines;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // 监听 DialogueSubsystem 的 InteractPressed
    UFUNCTION()
    void OnInteractPressed(AYJL_PlayerCharacter* Player);

    bool bPlayerInRange = false;

    // 启动对话；子类可覆盖
    virtual void StartDialogue();

    // 当前所有台词转为 FYJLDialogueLine
    void BuildLines(const TArray<FString>& Raw, TArray<FYJLDialogueLine>& Out) const;
};
