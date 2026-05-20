// YJL_PressurePlate.cpp
#include "YJL_PressurePlate.h"

#include "../YJL_Common.h"
#include "YJL_CarryableComponent.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

AYJL_PressurePlate::AYJL_PressurePlate()
{
    PrimaryActorTick.bCanEverTick = true;

    // === BaseBody：板子本体的物理碰撞（让玩家、物体能踩在上面）===
    // 默认尺寸：240cm x 240cm x 10cm（板面，2.4m 见方，10cm 厚）
    BaseBody = CreateDefaultSubobject<UBoxComponent>(TEXT("YJL_PlateBase"));
    SetRootComponent(BaseBody);
    BaseBody->InitBoxExtent(FVector(120.0f, 120.0f, 5.0f));
    BaseBody->SetCollisionProfileName(TEXT("BlockAll"));
    BaseBody->SetCollisionObjectType(ECC_WorldStatic);

    // === VisualMesh：视觉表现（按下时下沉）===
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YJL_PlateVisual"));
    VisualMesh->SetupAttachment(BaseBody);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 视觉只显示，碰撞交给 BaseBody

    {
        // 用 Cylinder 当默认视觉（直径 100cm × 高 100cm，缩成 240×240×10 见方）
        // Cube 边长 100，缩放 (2.4, 2.4, 0.1) → 240×240×10
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeFinder.Succeeded())
        {
            VisualMesh->SetStaticMesh(CubeFinder.Object);
            VisualMesh->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.1f));
        }
    }

    // === TriggerVolume：触发器（板面上方薄层，检测谁踩上来）===
    // 略大于板面（让箱子边缘也能被检测到），厚度 40cm（覆盖玩家脚部上方一点）
    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("YJL_PlateTrigger"));
    TriggerVolume->SetupAttachment(BaseBody);
    TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f)); // 在板上方 25cm
    TriggerVolume->InitBoxExtent(FVector(130.0f, 130.0f, 20.0f));   // 微微比板大
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
    TriggerVolume->SetGenerateOverlapEvents(true);
}

void AYJL_PressurePlate::BeginPlay()
{
    Super::BeginPlay();

    // 记录静止 Z（让 VisualMesh 按 RelativeLocation.Z 上下浮动）
    if (VisualMesh)
    {
        VisualRestZ = VisualMesh->GetRelativeLocation().Z;
        VisualTargetZ = VisualRestZ;
    }

    if (TriggerVolume)
    {
        TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AYJL_PressurePlate::HandleTriggerBeginOverlap);
        TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AYJL_PressurePlate::HandleTriggerEndOverlap);
    }

    // 启动时先评估一次（万一开始时就有物体压着）
    Evaluate();
}

void AYJL_PressurePlate::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!VisualMesh) return;

    // VisualMesh 沿 Z 朝 VisualTargetZ 插值
    const FVector L = VisualMesh->GetRelativeLocation();
    const float NewZ = FMath::FInterpTo(L.Z, VisualTargetZ, DeltaSeconds, PressInterpSpeed);
    if (!FMath::IsNearlyEqual(NewZ, L.Z, 0.01f))
    {
        VisualMesh->SetRelativeLocation(FVector(L.X, L.Y, NewZ));
    }
}

void AYJL_PressurePlate::HandleTriggerBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
    if (OtherActor == this) return;
    Evaluate();
}

void AYJL_PressurePlate::HandleTriggerEndOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/)
{
    if (OtherActor == this) return;
    Evaluate();
}

void AYJL_PressurePlate::Evaluate()
{
    if (!TriggerVolume) return;

    // 取所有当前 Overlap 的 Actor，累加重量
    TArray<AActor*> Overlapping;
    TriggerVolume->GetOverlappingActors(Overlapping);

    float Total = 0.0f;
    for (AActor* A : Overlapping)
    {
        if (!A || A == this) continue;
        Total += WeightOf(A);
    }

    if (!bIsActive && Total >= ActivationWeight)
    {
        SetActive(true);
    }
    else if (bIsActive && !bActivationLatched && Total <= DeactivationWeight)
    {
        SetActive(false);
    }
}

float AYJL_PressurePlate::WeightOf(AActor* Other) const
{
    if (!Other) return 0.0f;

    // 玩家：ACharacter 一律按 PlayerWeight
    if (Other->IsA(ACharacter::StaticClass()))
    {
        return PlayerWeight;
    }

    // 带 Carryable 组件的 → 用组件 Weight 字段
    if (UYJL_CarryableComponent* C = Other->FindComponentByClass<UYJL_CarryableComponent>())
    {
        return C->Weight;
    }

    // 其他物体不计入
    return 0.0f;
}

void AYJL_PressurePlate::SetActive(bool bNewActive)
{
    if (bIsActive == bNewActive) return;
    bIsActive = bNewActive;

    // 视觉目标：按下 = 静止 - PressDepth；弹起 = 静止
    VisualTargetZ = bIsActive ? (VisualRestZ - PressDepth) : VisualRestZ;

    if (bScreenLog)
    {
        YJL_SCREEN(2.0f, bIsActive ? FColor::Green : FColor::Yellow,
            "[YJL] PressurePlate %s -> %s", *GetName(), bIsActive ? TEXT("Activated") : TEXT("Deactivated"));
    }

    if (bIsActive)
    {
        OnActivated.Broadcast(this);
    }
    else
    {
        OnDeactivated.Broadcast(this);
    }
}

void AYJL_PressurePlate::ResetPlate()
{
    SetActive(false);
}
