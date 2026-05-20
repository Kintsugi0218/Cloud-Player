// YJL_BearNpcActor.cpp
#include "YJL_BearNpcActor.h"
#include "../YJL_Common.h"

// 在 GameState cpp 中声明的工具
extern bool YJL_GameState_IsFormUnlocked(UWorld* World, FName FormId);
extern void YJL_GameState_UnlockForm(UWorld* World, FName FormId);

AYJL_BearNpcActor::AYJL_BearNpcActor()
{
    NpcName = TEXT("小熊");

    LinesFirst = {
        TEXT("嗨，旅行者……你看起来有点疲倦。"),
        TEXT("这片云之地有些秘密，需要更柔软、更结实的身体才能解开。"),
        TEXT("接受我的祝福吧——你将获得一个新的形态：小熊。"),
        TEXT("变成小熊时，你能抱起重物，把它们搬到该去的地方。"),
        TEXT("按 Q 或 E 切换形态，按住一秒钟就能完成变身。去试试吧！"),
    };
    LinesAfter = {
        TEXT("小熊形态用起来还顺手吗？"),
        TEXT("记得，重的东西可以搬到该去的地方。"),
    };
}

void AYJL_BearNpcActor::StartDialogue()
{
    auto* GI = GetGameInstance();
    if (!GI) return;
    auto* DS = GI->GetSubsystem<UYJL_DialogueSubsystem>();
    if (!DS) return;

    const bool bAlreadyUnlocked = YJL_GameState_IsFormUnlocked(GetWorld(), FName("bear"));
    const TArray<FString>& Raw = bAlreadyUnlocked ? LinesAfter : LinesFirst;

    TArray<FYJLDialogueLine> Built;
    BuildLines(Raw, Built);

    FYJLDialogueFinishedDelegate Cb;
    if (!bAlreadyUnlocked)
    {
        Cb.BindDynamic(this, &AYJL_BearNpcActor::OnDialogueCompleteUnlockBear);
    }
    DS->StartDialogue(Built, Cb);
}

void AYJL_BearNpcActor::OnDialogueCompleteUnlockBear()
{
    YJL_GameState_UnlockForm(GetWorld(), FName("bear"));
    YJL_SCREEN(3.0f, FColor::Yellow, "[YJL] 小熊形态已解锁！按 Q/E 切换试试");
}
