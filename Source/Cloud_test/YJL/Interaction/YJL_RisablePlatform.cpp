// YJL_RisablePlatform.cpp
#include "YJL_RisablePlatform.h"

#include "../YJL_Common.h"
#include "YJL_PressurePlate.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AYJL_RisablePlatform::AYJL_RisablePlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YJL_PlatformMesh"));
    SetRootComponent(PlatformMesh);

    // 默认形状：300×300×50cm 的方台（在 BP 里可换 Mesh / 调 Scale）
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeFinder.Succeeded())
        {
            PlatformMesh->SetStaticMesh(CubeFinder.Object);
            PlatformMesh->SetRelativeScale3D(FVector(3.0f, 3.0f, 0.5f));
        }
    }

    // 物理碰撞：BlockAll，让玩家能站在上面
    PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
    PlatformMesh->SetCollisionObjectType(ECC_WorldStatic);
    // 必须 Movable 才能在运行时改变位置
    PlatformMesh->SetMobility(EComponentMobility::Movable);
}

void AYJL_RisablePlatform::BeginPlay()
{
    Super::BeginPlay();

    InitialZ = GetActorLocation().Z;
    TargetZ = InitialZ + LoweredZRelative;

    // 摆放时直接处于"下降态"
    FVector Loc = GetActorLocation();
    Loc.Z = TargetZ;
    SetActorLocation(Loc);

    // 订阅关联压力板的信号
    if (LinkedPlate)
    {
        LinkedPlate->OnActivated.AddDynamic(this, &AYJL_RisablePlatform::HandlePlateActivated);
        LinkedPlate->OnDeactivated.AddDynamic(this, &AYJL_RisablePlatform::HandlePlateDeactivated);

        if (bScreenLog)
        {
            YJL_SCREEN(3.0f, FColor::Cyan, "[YJL] Platform %s -> linked plate %s",
                *GetName(), *LinkedPlate->GetName());
        }
    }
    else if (bScreenLog)
    {
        YJL_SCREEN(5.0f, FColor::Yellow, "[YJL] Platform %s: LinkedPlate 未设置（只能通过外部 Blueprint 触发）",
            *GetName());
    }
}

void AYJL_RisablePlatform::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (LinkedPlate)
    {
        LinkedPlate->OnActivated.RemoveDynamic(this, &AYJL_RisablePlatform::HandlePlateActivated);
        LinkedPlate->OnDeactivated.RemoveDynamic(this, &AYJL_RisablePlatform::HandlePlateDeactivated);
    }
    Super::EndPlay(EndPlayReason);
}

void AYJL_RisablePlatform::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const FVector Loc = GetActorLocation();
    const float NewZ = FMath::FInterpTo(Loc.Z, TargetZ, DeltaSeconds, TransitionInterpSpeed);
    if (!FMath::IsNearlyEqual(NewZ, Loc.Z, 0.01f))
    {
        // 用 Sweep=true 让玩家被推开/带起（避免穿模）
        FHitResult Hit;
        SetActorLocation(FVector(Loc.X, Loc.Y, NewZ), /*bSweep*/ true, &Hit);
    }
}

void AYJL_RisablePlatform::RaiseUp()
{
    if (bIsRaised) return;
    bIsRaised = true;
    TargetZ = InitialZ + RaisedZRelative;
    if (bScreenLog)
    {
        YJL_SCREEN(2.0f, FColor::Green, "[YJL] Platform %s -> Raise (targetZ=%.0f)", *GetName(), TargetZ);
    }
}

void AYJL_RisablePlatform::LowerDown()
{
    if (!bIsRaised) return;
    bIsRaised = false;
    TargetZ = InitialZ + LoweredZRelative;
    if (bScreenLog)
    {
        YJL_SCREEN(2.0f, FColor::Yellow, "[YJL] Platform %s -> Lower (targetZ=%.0f)", *GetName(), TargetZ);
    }
}

void AYJL_RisablePlatform::HandlePlateActivated(AYJL_PressurePlate* /*Plate*/)
{
    RaiseUp();
}

void AYJL_RisablePlatform::HandlePlateDeactivated(AYJL_PressurePlate* /*Plate*/)
{
    if (bLowerOnDeactivate)
    {
        LowerDown();
    }
}
