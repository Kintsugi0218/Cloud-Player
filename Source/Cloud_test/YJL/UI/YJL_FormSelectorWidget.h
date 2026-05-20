// YJL_FormSelectorWidget.h
// 形态选择头顶 UI。
// 对应 Godot 项目的 FormSelectorUI。
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../Data/YJL_FormDefinition.h"
#include "YJL_FormSelectorWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;

UCLASS()
class CLOUD_TEST_API UYJL_FormSelectorWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    // UMG 蓝图控件绑定，名称和类型必须在蓝图中完全一致
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> FormName;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> ChargeBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Swatch;

    // 绑定至 FormManagerComponent 派发的代理回调
    UFUNCTION()
    void HandleFormChargeChanged(FName PendingId, float ChargeAlpha);

    UFUNCTION()
    void HandleFormPanelHidden();

    UFUNCTION()
    void HandleFormChanged(const FYJLFormDefinition& NewForm);
};
