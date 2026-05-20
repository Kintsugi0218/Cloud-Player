// YJL_FormManagerComponent.cpp
#include "YJL_FormManagerComponent.h"

#include "../YJL_Common.h"
#include "../Character/YJL_PlayerCharacter.h"
#include "../Data/YJL_GameStateSubsystem.h"
#include "YJL_CarryComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

// 在另一个 cpp 里声明的工具：操作 GameState 已解锁列表
extern void YJL_GameState_GetUnlockedForms(UWorld* World, TArray<FName>& OutUnlocked);
extern void YJL_GameState_EnsureDefaultUnlocked(UWorld* World);

UYJL_FormManagerComponent::UYJL_FormManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UYJL_FormManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    OwnerPlayer = Cast<AYJL_PlayerCharacter>(GetOwner());
    if (OwnerPlayer)
    {
        CarryRef = OwnerPlayer->CarryComp;
    }

    InitDefaultForms();

    YJL_GameState_EnsureDefaultUnlocked(GetWorld());
    RefreshUnlockedFormsFromGameState();

    // 订阅 GameState 的解锁广播，这样 BearNPC 对话结束解锁 bear 后我们能立刻刷新 UnlockedOrder
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UYJL_GameStateSubsystem* GS = GI->GetSubsystem<UYJL_GameStateSubsystem>())
        {
            GS->OnFormUnlocked.AddDynamic(this, &UYJL_FormManagerComponent::HandleFormUnlocked);
        }
    }

    // 应用 default 形态
    if (UnlockedOrder.Num() > 0)
    {
        ApplyFormById(UnlockedOrder[0]);
    }
}

void UYJL_FormManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UYJL_GameStateSubsystem* GS = GI->GetSubsystem<UYJL_GameStateSubsystem>())
        {
            GS->OnFormUnlocked.RemoveDynamic(this, &UYJL_FormManagerComponent::HandleFormUnlocked);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UYJL_FormManagerComponent::HandleFormUnlocked(FName FormId)
{
    RefreshUnlockedFormsFromGameState();
    YJL_SCREEN(2.0f, FColor::Green, "[YJL] FormManager 已刷新解锁列表：当前 %d 个形态可用", UnlockedOrder.Num());
}

void UYJL_FormManagerComponent::InitDefaultForms()
{
    Registry.Empty();

    // ----- default 形态 -----
    {
        FYJLFormDefinition F;
        F.Id = FName("default");
        F.DisplayName = TEXT("冒险者");
        F.UIColor = FLinearColor(0.3f, 0.6f, 1.0f, 1.0f);
        F.Speed = 600.0f;
        F.Acceleration = 4000.0f;
        F.BrakingDeceleration = 5000.0f;
        F.bCanJump = true;
        F.JumpHeight = 200.0f;
        F.TimeToPeak = 0.4f;
        F.TimeToDescent = 0.35f;
        F.SpecialAction = EYJLFormSpecialAction::None;
        Registry.Add(F.Id, F);
    }

    // ----- bear 形态 -----
    {
        FYJLFormDefinition F;
        F.Id = FName("bear");
        F.DisplayName = TEXT("小熊");
        F.UIColor = FLinearColor(0.55f, 0.35f, 0.18f, 1.0f);
        F.Speed = 600.0f;
        F.Acceleration = 4000.0f;
        F.BrakingDeceleration = 5000.0f;
        F.bCanJump = false;
        F.JumpHeight = 200.0f;
        F.TimeToPeak = 0.4f;
        F.TimeToDescent = 0.35f;
        F.SpecialAction = EYJLFormSpecialAction::GrabDrop;
        Registry.Add(F.Id, F);
    }
}

void UYJL_FormManagerComponent::RefreshUnlockedFormsFromGameState()
{
    TArray<FName> Unlocked;
    YJL_GameState_GetUnlockedForms(GetWorld(), Unlocked);

    UnlockedOrder.Empty();
    // 只保留已注册的
    for (FName Id : Unlocked)
    {
        if (Registry.Contains(Id))
        {
            UnlockedOrder.Add(Id);
        }
    }
    // 若空，至少塞 default
    if (UnlockedOrder.Num() == 0 && Registry.Contains(FName("default")))
    {
        UnlockedOrder.Add(FName("default"));
    }
}

void UYJL_FormManagerComponent::ApplyFormById(FName FormId)
{
    const FYJLFormDefinition* Found = Registry.Find(FormId);
    if (!Found)
    {
        YJL_SCREEN(3.0f, FColor::Red, "[YJL] ApplyFormById: 未知形态 %s", *FormId.ToString());
        return;
    }
    ApplyForm(*Found);
}

void UYJL_FormManagerComponent::ApplyForm(const FYJLFormDefinition& Form)
{
    CurrentForm = Form;

    if (OwnerPlayer)
    {
        OwnerPlayer->ApplyFormStats(Form);
    }
    if (CarryRef)
    {
        CarryRef->SetActive_YJL(Form.SpecialAction == EYJLFormSpecialAction::GrabDrop);
    }
    OnFormChanged.Broadcast(Form);

    YJL_SCREEN(2.0f, FColor::Green, "[YJL] 形态变更 → %s", *Form.DisplayName);
}

int32 UYJL_FormManagerComponent::IndexOf(FName Id) const
{
    return UnlockedOrder.IndexOfByKey(Id);
}

void UYJL_FormManagerComponent::PickPrev() { Step(-1); }
void UYJL_FormManagerComponent::PickNext() { Step(+1); }

void UYJL_FormManagerComponent::Step(int32 Direction)
{
    if (bIsTransforming) return;
    if (UnlockedOrder.Num() == 0)
    {
        RefreshUnlockedFormsFromGameState();
        if (UnlockedOrder.Num() == 0) return;
    }

    // 当前 pending 不在已解锁里则重置到当前形态
    int32 Idx = IndexOf(PendingId);
    if (Idx < 0)
    {
        Idx = IndexOf(CurrentForm.Id);
        if (Idx < 0) Idx = 0;
    }
    Idx = (Idx + Direction + UnlockedOrder.Num()) % UnlockedOrder.Num();
    PendingId = UnlockedOrder[Idx];

    ChargeTimer = 0.0f;
    IdleTimer = 0.0f;
    bPanelVisible = true;

    // 通知 UI 当前 pending（蓄力 0）
    OnFormChargeChanged.Broadcast(PendingId, 0.0f);
    YJL_SCREEN(0.6f, FColor::White, "[YJL] Pending → %s", *PendingId.ToString());
}

void UYJL_FormManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bPanelVisible || bIsTransforming) return;
    if (PendingId.IsNone()) return;

    const bool bSameAsCurrent = (PendingId == CurrentForm.Id);
    if (bSameAsCurrent)
    {
        // pending == current：不蓄力，倒计时后隐藏
        IdleTimer += DeltaTime;
        if (IdleTimer >= IdleDismissTime)
        {
            bPanelVisible = false;
            IdleTimer = 0.0f;
            OnFormPanelHidden.Broadcast();
        }
    }
    else
    {
        IdleTimer = 0.0f;
        ChargeTimer += DeltaTime;
        const float Alpha = FMath::Clamp(ChargeTimer / FMath::Max(ChargeTime, 0.01f), 0.0f, 1.0f);
        OnFormChargeChanged.Broadcast(PendingId, Alpha);

        if (ChargeTimer >= ChargeTime)
        {
            bIsTransforming = true;
            ApplyFormById(PendingId);
            bIsTransforming = false;
            ChargeTimer = 0.0f;
            IdleTimer = 0.0f;
            // 变身完成后保留面板一小会再隐藏
            // 通过 IdleTimer 路径自动隐藏
        }
    }
}

void UYJL_FormManagerComponent::TrySpecialAction()
{
    switch (CurrentForm.SpecialAction)
    {
    case EYJLFormSpecialAction::GrabDrop:
        if (CarryRef) CarryRef->TryGrabOrDrop();
        break;
    default:
        break;
    }
}

bool UYJL_FormManagerComponent::FindFormDefinition(FName FormId, FYJLFormDefinition& OutForm) const
{
    if (const FYJLFormDefinition* Found = Registry.Find(FormId))
    {
        OutForm = *Found;
        return true;
    }
    return false;
}
