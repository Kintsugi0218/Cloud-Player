#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MyMorphDataAsset.h" // 引入新类
#include "MyPlayerCharacter.generated.h"

class USkeletalMeshComponent; // 新增，用于动态形态

// Enhanced Input
class UInputMappingContext;
class UInputAction;

// Camera Rig
class AMyCameraRigActor;
class UDialogueWidget;

UCLASS()
class CLOUD_TEST_API AMyPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMyPlayerCharacter();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // --- 新增的变身相关接口 ---
    UFUNCTION(BlueprintCallable, Category = "Morphing")
    bool SwitchMorph(UMyMorphDataAsset* NewMorphData); // 切换至指定形态

    UFUNCTION(BlueprintCallable, Category = "Morphing")
    void UnlockMorphByTag(FName Tag); // 解锁形态

    // ===== 变身系统核心变量 =====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morphing")
    TArray<TObjectPtr<UMyMorphDataAsset>> AllMorphDataList; // 轮换顺序来源,所有可用形态

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morphing", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMyMorphDataAsset> DefaultMorphData; // 默认形态

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morphing")
    UMyMorphDataAsset* CurrentMorphData; // 当前形态数据

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morphing")
    TArray<FName> UnlockedMorphTags; // 解锁的形态


    // ===== 交互相关 =====
    void Interact();

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> InteractPromptClass;

    UPROPERTY()
    UUserWidget* InteractPromptWidget;

    UPROPERTY()
    AActor* CurrentInteractable = nullptr;

    void SetCurrentInteractable(AActor* NewInteractable);
    void ShowInteractPrompt();
    void HideInteractPrompt();

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> DialogueWidgetClass;

    UPROPERTY()
    UDialogueWidget* DialogueWidget;
    void StartDialogue(const TArray<FString>& Lines);
    UFUNCTION(BlueprintCallable)
    void EndDialogue();

private:
    // ===== Enhanced Input callbacks =====
    void Move(const FInputActionValue& Value);
    void JumpStarted(const FInputActionValue& Value);
    void JumpCompleted(const FInputActionValue& Value);

    void OnPrevMorphPressed();
    void OnNextMorphPressed();

    bool CycleMorph(int32 Direction); // -1:上一个, +1:下一个
    void BuildUnlockedMorphList(TArray<UMyMorphDataAsset*>& OutList) const; // 用已解锁的形态填充OutList

    // ===== 通用技能槽 callbacks =====
    void OnAbilitySlotPressed();

    void OnAbilitySlotReleased();

    void DispatchAbilityInputPressed(int32 SlotIndex);
    void DispatchAbilityInputReleased(int32 SlotIndex);

    // ===== Enhanced Input assets (在蓝图里指定) =====
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* InteractAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* PrevMorphAction; // Q

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* NextMorphAction; // E

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* AbilitySlotAction;

    // ===== Camera Rig =====
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CameraRig", meta = (AllowPrivateAccess = "true"))
    bool bAutoBindCameraRig = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CameraRig", meta = (AllowPrivateAccess = "true"))
    AMyCameraRigActor* CameraRigRef = nullptr;

    UPROPERTY()
    TArray<UActorComponent*> ActiveMorphAbilities; // 当前形态激活的特殊能力组件

    UPROPERTY()
    TObjectPtr<class UInputMappingContext> ActiveMorphMappingContext = nullptr;

    // ===== 从 MorphData 同步过来的临时变量 (用于Tick) =====
    float CurrentMaxJumpHeight = 200.f;
    float CurrentJumpSpeed = 300.f;

    // ===== Jump Logic (从头文件移至此处) =====
    bool bJumpHeld = false;
    bool bIsJumping = false;
    float JumpStartZ = 0.f;

    // ===== 升空体积缩小相关 =====
    float TargetScale = 1.f;
    float ScaleInterpSpeed = 5.f;

    // ===== 变身系统辅助函数 =====
    void ApplyMorphSettings(UMyMorphDataAsset* Data);
    void RefreshMorphAbilities(UMyMorphDataAsset* Data);
    void RefreshMorphInputContext(UMyMorphDataAsset* Data);
};