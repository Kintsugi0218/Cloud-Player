// YJL_DialogueSubsystem.cpp
#include "YJL_DialogueSubsystem.h"

#include "../YJL_Common.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UYJL_DialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bIsActive = false;
    LineIndex = 0;
}

void UYJL_DialogueSubsystem::Deinitialize()
{
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(RenderTimer);
    }
    Super::Deinitialize();
}

void UYJL_DialogueSubsystem::StartDialogue(const TArray<FYJLDialogueLine>& InLines, const FYJLDialogueFinishedDelegate& OnFinished)
{
    if (bIsActive) return;
    if (InLines.Num() == 0) return;

    Lines = InLines;
    LineIndex = 0;
    OnFinishedCb = OnFinished;
    bIsActive = true;

    DialogueStarted.Broadcast();
    RenderCurrentLine();

    // 每 0.1s 重新把当前台词推到屏幕上（防止 AddOnScreenDebugMessage 自然消失）
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().SetTimer(RenderTimer, FTimerDelegate::CreateUObject(this, &UYJL_DialogueSubsystem::TickRender), 0.1f, true);
    }
}

void UYJL_DialogueSubsystem::Advance()
{
    if (!bIsActive) return;
    LineIndex++;
    if (LineIndex >= Lines.Num())
    {
        EndDialogue();
        return;
    }
    RenderCurrentLine();
}

void UYJL_DialogueSubsystem::EndDialogue()
{
    bIsActive = false;
    if (UWorld* W = GetWorld())
    {
        W->GetTimerManager().ClearTimer(RenderTimer);
    }
    if (GEngine)
    {
        // 清掉对话框那一行的 key
        GEngine->RemoveOnScreenDebugMessage(900001);
        GEngine->RemoveOnScreenDebugMessage(900002);
    }
    DialogueFinished.Broadcast();

    if (OnFinishedCb.IsBound())
    {
        FYJLDialogueFinishedDelegate Cb = OnFinishedCb;
        OnFinishedCb.Unbind();
        Cb.Execute();
    }
}

void UYJL_DialogueSubsystem::RenderCurrentLine()
{
    TickRender();
}

void UYJL_DialogueSubsystem::TickRender()
{
    if (!bIsActive || !GEngine) return;
    if (!Lines.IsValidIndex(LineIndex)) return;
    const FYJLDialogueLine& L = Lines[LineIndex];

    const FString Header = FString::Printf(TEXT("【%s】"), *L.Speaker);
    const FString Body = FString::Printf(TEXT("%s    （%d/%d  按 F 继续）"), *L.Text, LineIndex + 1, Lines.Num());

    GEngine->AddOnScreenDebugMessage(900001, 0.3f, FColor::Cyan, Header, true, FVector2D(1.4f, 1.4f));
    GEngine->AddOnScreenDebugMessage(900002, 0.3f, FColor::White, Body, true, FVector2D(1.2f, 1.2f));
}

void UYJL_DialogueSubsystem::BroadcastInteractPressed(AYJL_PlayerCharacter* Player)
{
    InteractPressed.Broadcast(Player);
}
