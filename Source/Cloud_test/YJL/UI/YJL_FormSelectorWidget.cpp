// YJL_FormSelectorWidget.cpp
#include "YJL_FormSelectorWidget.h"
#include "../Character/YJL_PlayerCharacter.h"
#include "../Components/YJL_FormManagerComponent.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Engine/World.h"

void UYJL_FormSelectorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    AYJL_PlayerCharacter* PlayerChar = PC ? Cast<AYJL_PlayerCharacter>(PC->GetPawn()) : nullptr;
    if (PlayerChar && PlayerChar->FormManager)
    {
        // 绑定 FormManager 的多播委托，使用 AddUniqueDynamic 保证不重复绑定
        PlayerChar->FormManager->OnFormChargeChanged.AddUniqueDynamic(this, &UYJL_FormSelectorWidget::HandleFormChargeChanged);
        PlayerChar->FormManager->OnFormPanelHidden.AddUniqueDynamic(this, &UYJL_FormSelectorWidget::HandleFormPanelHidden);
        PlayerChar->FormManager->OnFormChanged.AddUniqueDynamic(this, &UYJL_FormSelectorWidget::HandleFormChanged);

        // 默认初始化时隐藏 UI
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UYJL_FormSelectorWidget::HandleFormChargeChanged(FName PendingId, float ChargeAlpha)
{
    // 显示 UI
    if (GetVisibility() != ESlateVisibility::HitTestInvisible)
    {
        SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    AYJL_PlayerCharacter* PlayerChar = PC ? Cast<AYJL_PlayerCharacter>(PC->GetPawn()) : nullptr;
    if (PlayerChar && PlayerChar->FormManager)
    {
        FYJLFormDefinition FormDef;
        if (PlayerChar->FormManager->FindFormDefinition(PendingId, FormDef))
        {
            if (FormName)
            {
                FormName->SetText(FText::FromString(FormDef.DisplayName));
            }
            if (Swatch)
            {
                Swatch->SetColorAndOpacity(FormDef.UIColor);
            }
        }
    }

    if (ChargeBar)
    {
        ChargeBar->SetPercent(ChargeAlpha);
    }
}

void UYJL_FormSelectorWidget::HandleFormPanelHidden()
{
    // 隐藏折叠 UI
    SetVisibility(ESlateVisibility::Collapsed);
}

void UYJL_FormSelectorWidget::HandleFormChanged(const FYJLFormDefinition& NewForm)
{
    // 预留接口，如果变身瞬间需要做额外动效可以在此实现
}
