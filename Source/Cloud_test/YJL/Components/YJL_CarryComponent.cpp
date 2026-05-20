// YJL_CarryComponent.cpp
#include "YJL_CarryComponent.h"

#include "../YJL_Common.h"
#include "../Character/YJL_PlayerCharacter.h"
#include "../Interaction/YJL_CarryableComponent.h"

#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UYJL_CarryComponent::UYJL_CarryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UYJL_CarryComponent::BeginPlay()
{
    Super::BeginPlay();
    OwnerPlayer = Cast<AYJL_PlayerCharacter>(GetOwner());
    if (!OwnerPlayer) return;

    // 创建探测体（球形 Overlap）
    Detector = NewObject<USphereComponent>(OwnerPlayer, TEXT("YJL_CarryDetector"));
    Detector->RegisterComponent();
    Detector->AttachToComponent(OwnerPlayer->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Detector->SetSphereRadius(DetectRadiusCm);
    Detector->SetRelativeLocation(DetectOffset);
    // 关键：用 OverlapAll Profile，否则默认会 Block 而非 Overlap，GetOverlappingActors 拿不到东西
    Detector->SetCollisionProfileName(TEXT("OverlapAll"));
    Detector->SetGenerateOverlapEvents(true);
    Detector->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 默认关闭，SetActive 时打开

    // 创建挂载点
    CarrySlot = NewObject<USceneComponent>(OwnerPlayer, TEXT("YJL_CarrySlot"));
    CarrySlot->RegisterComponent();
    CarrySlot->AttachToComponent(OwnerPlayer->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    CarrySlot->SetRelativeLocation(SlotOffset);
}

void UYJL_CarryComponent::SetActive_YJL(bool bInActive)
{
    bActive = bInActive;

    if (!bActive)
    {
        if (Held) DropHeld();
        Candidate = nullptr;
        if (Detector) Detector->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OnHintChanged.Broadcast(TEXT(""));
        YJL_SCREEN(1.0f, FColor::Silver, "[YJL] CarryComp 失活");
    }
    else
    {
        if (Detector) Detector->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        YJL_SCREEN(1.0f, FColor::Silver, "[YJL] CarryComp 激活");
    }
}

void UYJL_CarryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bActive) return;
    if (!Detector || !CarrySlot || !OwnerPlayer) return;

    // 每帧更新候选 + 提示
    if (Held)
    {
        UpdateHint();
        return;
    }
    Candidate = FindNearestCandidate();
    UpdateHint();
}

UYJL_CarryableComponent* UYJL_CarryComponent::FindNearestCandidate() const
{
    if (!Detector) return nullptr;

    TArray<AActor*> Overlapping;
    Detector->GetOverlappingActors(Overlapping);

    UYJL_CarryableComponent* Best = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();
    const FVector DetPos = Detector->GetComponentLocation();

    for (AActor* A : Overlapping)
    {
        if (!A || A == OwnerPlayer) continue;
        UYJL_CarryableComponent* C = A->FindComponentByClass<UYJL_CarryableComponent>();
        if (!C) continue;
        const float D = FVector::DistSquared(DetPos, A->GetActorLocation());
        if (D < BestDistSq)
        {
            BestDistSq = D;
            Best = C;
        }
    }
    return Best;
}

void UYJL_CarryComponent::UpdateHint()
{
    FString H;
    if (Held)
    {
        H = FString::Printf(TEXT("[空格] 放下%s"), *Held->DisplayName);
    }
    else if (Candidate)
    {
        H = FString::Printf(TEXT("[空格] 抓起%s"), *Candidate->DisplayName);
    }
    else
    {
        H = TEXT("");
    }
    OnHintChanged.Broadcast(H);
    if (!H.IsEmpty())
    {
        // 屏幕中央提示一行（覆盖型）
        if (GEngine) GEngine->AddOnScreenDebugMessage((uint64)(SIZE_T)this + 1, 0.2f, FColor::Yellow, H);
    }
}

bool UYJL_CarryComponent::TryGrabOrDrop()
{
    if (!bActive)
    {
        YJL_SCREEN(2.0f, FColor::Orange, "[YJL] TryGrabOrDrop: CarryComp 未激活（需要小熊形态）");
        return false;
    }
    if (Held) { DropHeld(); return true; }
    if (Candidate) { PickUp(Candidate); return true; }
    YJL_SCREEN(2.0f, FColor::Orange, "[YJL] TryGrabOrDrop: 周围没有可抓物（探测半径 %.0f cm）", DetectRadiusCm);
    return false;
}

void UYJL_CarryComponent::PickUp(UYJL_CarryableComponent* Carryable)
{
    if (!Carryable || Held) return;
    UPrimitiveComponent* Body = Carryable->GetPhysicsRoot();
    if (!Body)
    {
        YJL_SCREEN(4.0f, FColor::Red, "[YJL] PickUp 失败：%s 的 RootComponent 不是 PrimitiveComponent（StaticMesh）。请在 BP_YJL_Carryable 里把 StaticMesh 设为根节点", *Carryable->GetOwner()->GetName());
        return;
    }

    // 记录原物理状态
    bCachedSimulatePhysics = Body->IsSimulatingPhysics();
    CachedCollisionEnabled = Body->GetCollisionEnabled();

    // 关物理 + 关碰撞（避免推开玩家）
    Body->SetSimulatePhysics(false);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Attach 到 CarrySlot，保持世界变换；然后清零相对位置
    Body->AttachToComponent(CarrySlot, FAttachmentTransformRules::KeepWorldTransform);
    Body->SetRelativeLocation(Carryable->CarryOffset);
    if (Carryable->bKeepUpright)
    {
        Body->SetRelativeRotation(FRotator::ZeroRotator);
    }

    Held = Carryable;
    YJL_SCREEN(1.5f, FColor::Green, "[YJL] 抓起：%s", *Carryable->DisplayName);
}

void UYJL_CarryComponent::DropHeld()
{
    if (!Held) return;
    UPrimitiveComponent* Body = Held->GetPhysicsRoot();
    if (!Body) { Held = nullptr; return; }

    // Detach 保持世界变换
    Body->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

    // 放到玩家脚下前方一点
    if (OwnerPlayer)
    {
        FVector Drop = CarrySlot ? CarrySlot->GetComponentLocation() : OwnerPlayer->GetActorLocation() + OwnerPlayer->GetActorForwardVector() * 100.0f;
        Drop.Z = OwnerPlayer->GetActorLocation().Z + 50.0f;
        Body->SetWorldLocation(Drop);
        if (Held->bKeepUpright)
        {
            Body->SetWorldRotation(FRotator::ZeroRotator);
        }
    }

    // 恢复物理
    Body->SetCollisionEnabled(CachedCollisionEnabled);
    Body->SetSimulatePhysics(bCachedSimulatePhysics);
    if (Body->IsSimulatingPhysics())
    {
        Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }

    YJL_SCREEN(1.5f, FColor::Green, "[YJL] 放下：%s", *Held->DisplayName);
    Held = nullptr;
}
