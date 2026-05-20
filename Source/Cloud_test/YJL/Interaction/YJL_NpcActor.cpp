// YJL_NpcActor.cpp
#include "YJL_NpcActor.h"

#include "../YJL_Common.h"
#include "../Character/YJL_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"

AYJL_NpcActor::AYJL_NpcActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 用 StaticMesh 当根：不依赖任何美术资产，运行时用引擎自带 Cube
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YJL_NpcBody"));
    SetRootComponent(Body);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionResponseToAllChannels(ECR_Block);

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("YJL_NpcTrigger"));
    Trigger->SetupAttachment(RootComponent);
    Trigger->SetBoxExtent(FVector(180.0f));
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetGenerateOverlapEvents(true);

    // 默认台词
    DialogLines = {
        TEXT("欢迎来到云之地，旅行者。"),
        TEXT("这里风景虽然安静，但藏着不少秘密。"),
        TEXT("祝你旅途顺利。"),
    };
}

void AYJL_NpcActor::BeginPlay()
{
    Super::BeginPlay();

    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AYJL_NpcActor::OnBeginOverlap);
    Trigger->OnComponentEndOverlap.AddDynamic(this, &AYJL_NpcActor::OnEndOverlap);

    // 给 Body 一个引擎内置 Cube + 默认材质（避免你建任何资产）
    if (Body && !Body->GetStaticMesh())
    {
        if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
        {
            Body->SetStaticMesh(Cube);
        }
        if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")))
        {
            Body->SetMaterial(0, Mat);
        }
        Body->SetWorldScale3D(FVector(1.0f, 1.0f, 1.8f));
    }

    // 订阅全局 InteractPressed
    if (auto* GI = GetGameInstance())
    {
        if (auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>())
        {
            DS->InteractPressed.AddDynamic(this, &AYJL_NpcActor::OnInteractPressed);
        }
    }
}

void AYJL_NpcActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (auto* GI = GetGameInstance())
    {
        if (auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>())
        {
            DS->InteractPressed.RemoveDynamic(this, &AYJL_NpcActor::OnInteractPressed);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AYJL_NpcActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 玩家在范围内 + 没有对话进行中 → 显示提示
    if (bPlayerInRange && GEngine)
    {
        bool bDialogueActive = false;
        if (auto* GI = GetGameInstance())
        {
            if (auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>())
            {
                bDialogueActive = DS->IsActive();
            }
        }
        if (!bDialogueActive)
        {
            const FString Hint = FString::Printf(TEXT("[F] 与 %s 对话"), *NpcName);
            GEngine->AddOnScreenDebugMessage((uint64)(SIZE_T)this, 0.2f, FColor::Yellow, Hint);
        }
    }
}

void AYJL_NpcActor::OnBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (Cast<AYJL_PlayerCharacter>(OtherActor))
    {
        bPlayerInRange = true;
    }
}

void AYJL_NpcActor::OnEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
    if (Cast<AYJL_PlayerCharacter>(OtherActor))
    {
        bPlayerInRange = false;
    }
}

void AYJL_NpcActor::OnInteractPressed(AYJL_PlayerCharacter* /*Player*/)
{
    if (!bPlayerInRange) return;
    auto* GI = GetGameInstance();
    if (!GI) return;
    auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>();
    if (!DS || DS->IsActive()) return;

    StartDialogue();
}

void AYJL_NpcActor::StartDialogue()
{
    if (auto* GI = GetGameInstance())
    {
        if (auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>())
        {
            TArray<FYJLDialogueLine> Built;
            BuildLines(DialogLines, Built);
            DS->StartDialogue(Built, FYJLDialogueFinishedDelegate());
        }
    }
}

void AYJL_NpcActor::BuildLines(const TArray<FString>& Raw, TArray<FYJLDialogueLine>& Out) const
{
    Out.Reset();
    for (const FString& S : Raw)
    {
        FYJLDialogueLine L;
        L.Speaker = NpcName;
        L.Text = S;
        Out.Add(L);
    }
}
