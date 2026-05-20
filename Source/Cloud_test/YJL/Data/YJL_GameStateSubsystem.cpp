// YJL_GameStateSubsystem.cpp
#include "YJL_GameStateSubsystem.h"

#include "../YJL_Common.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UYJL_GameStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UnlockedForms = { FName("default") };
    CoinCount = 0;
}

void UYJL_GameStateSubsystem::UnlockForm(FName FormId)
{
    if (UnlockedForms.Contains(FormId)) return;
    UnlockedForms.Add(FormId);
    OnFormUnlocked.Broadcast(FormId);
    YJL_SCREEN(2.0f, FColor::Yellow, "[YJL] GameState 解锁形态：%s", *FormId.ToString());
}

bool UYJL_GameStateSubsystem::IsFormUnlocked(FName FormId) const
{
    return UnlockedForms.Contains(FormId);
}

void UYJL_GameStateSubsystem::AddCoin(int32 Amount)
{
    CoinCount += Amount;
    OnCoinChanged.Broadcast(CoinCount);
}

// =========================================================
// 全局工具函数：避免其他 cpp 强依赖 GameInstance 子系统头文件
// =========================================================
void YJL_GameState_EnsureDefaultUnlocked(UWorld* World)
{
    if (!World) return;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;
    UYJL_GameStateSubsystem* S = GI->GetSubsystem<UYJL_GameStateSubsystem>();
    if (!S) return;
    if (!S->IsFormUnlocked(FName("default")))
    {
        S->UnlockForm(FName("default"));
    }
}

void YJL_GameState_GetUnlockedForms(UWorld* World, TArray<FName>& OutUnlocked)
{
    OutUnlocked.Reset();
    if (!World) return;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;
    UYJL_GameStateSubsystem* S = GI->GetSubsystem<UYJL_GameStateSubsystem>();
    if (!S) return;
    OutUnlocked = S->GetUnlockedForms();
}

void YJL_GameState_UnlockForm(UWorld* World, FName FormId)
{
    if (!World) return;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;
    UYJL_GameStateSubsystem* S = GI->GetSubsystem<UYJL_GameStateSubsystem>();
    if (!S) return;
    S->UnlockForm(FormId);
}

bool YJL_GameState_IsFormUnlocked(UWorld* World, FName FormId)
{
    if (!World) return false;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return false;
    UYJL_GameStateSubsystem* S = GI->GetSubsystem<UYJL_GameStateSubsystem>();
    if (!S) return false;
    return S->IsFormUnlocked(FormId);
}
