#include "MyPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h" // 新增
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h" // 用于异步加载
#include "Engine/LocalPlayer.h"


#include "InteractInterface.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.h"

// Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "MyMorphAbilityInputReceiver.h"

// Camera Rig
#include "MyCameraRigActor.h"

// Engine
#include "Engine/Engine.h" // 用于 GEngine->AddOnScreenDebugMessage

AMyPlayerCharacter::AMyPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- Capsule ---
    GetCapsuleComponent()->InitCapsuleSize(34.f, 50.f);

    // --- Character Movement (初始化为默认值，将在变身时被覆盖) ---
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = 450.f; // 初始值
    GetCharacterMovement()->AirControl = 0.6f; // 初始值
    GetCharacterMovement()->GravityScale = 0.35f; // 初始值

    UnlockedMorphTags = { FName(TEXT("Morph.Default")) };
}

void AMyPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 确保默认形态在列表第一个
    if (DefaultMorphData)
    {
        AllMorphDataList.Insert(DefaultMorphData, 0);
    }

    // 先添加默认输入上下文
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                if (DefaultMappingContext)
                {
                    Subsystem->AddMappingContext(DefaultMappingContext, 0);
                }
            }
        }
    }

    if (DefaultMorphData)
    {
        UnlockMorphByTag(DefaultMorphData->UnlockTag); // 将默认形态解锁
        SwitchMorph(DefaultMorphData); // 内部会应用参数+能力+形态输入
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AMyPlayerCharacter::BeginPlay - DefaultMorphData is not set! Character will use engine defaults."));
    }

    // 兜底：确保当前形态输入上下文已经正确挂载
    RefreshMorphInputContext(CurrentMorphData);

    // 绑定独立Camera Rig
    if (bAutoBindCameraRig)
    {
        if (!CameraRigRef)
        {
            CameraRigRef = Cast<AMyCameraRigActor>(UGameplayStatics::GetActorOfClass(this, AMyCameraRigActor::StaticClass()));
        }

        if (CameraRigRef)
        {
            CameraRigRef->SetFollowTarget(this);
        }
    }
}

void AMyPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bJumpHeld)
    {
        if (!bIsJumping && GetCharacterMovement()->IsMovingOnGround())
        {
            bIsJumping = true;
            JumpStartZ = GetActorLocation().Z;
            GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
        }

        if (bIsJumping)
        {
            const float CurrentZ = GetActorLocation().Z;
            const float JumpedHeight = CurrentZ - JumpStartZ;

            if (JumpedHeight < CurrentMaxJumpHeight) // 使用临时变量
            {
                FVector Vel = GetCharacterMovement()->Velocity;
                Vel.Z = CurrentJumpSpeed; // 使用临时变量
                GetCharacterMovement()->Velocity = Vel;
                float Alpha = FMath::Clamp(JumpedHeight / CurrentMaxJumpHeight, 0.f, 1.f);

                TargetScale = FMath::Lerp(1.f, 0.5f, Alpha);
            }
            else
            {
                bIsJumping = false;
            }
        }
    }


    if (GetCharacterMovement()->IsMovingOnGround())
    {
        bIsJumping = false;
        TargetScale = 1.f;
    }

    FVector CurrentScale = GetActorScale3D();
    float NewScale = FMath::FInterpTo(CurrentScale.X, TargetScale, DeltaTime, ScaleInterpSpeed);
    SetActorScale3D(FVector(NewScale));
}

void AMyPlayerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MoveValue = Value.Get<FVector2D>();
    if (!Controller) return;

    const APlayerController* PC = Cast<APlayerController>(Controller);
    if (!PC || !PC->PlayerCameraManager) return;

    const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
    const FRotator CamYaw(0.f, CamRot.Yaw, 0.f);

    const FVector Forward = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(CamYaw).GetUnitAxis(EAxis::Y);

    AddMovementInput(Forward, MoveValue.Y);
    AddMovementInput(Right, MoveValue.X);
}

void AMyPlayerCharacter::JumpStarted(const FInputActionValue& Value)
{
    bJumpHeld = true;
}

void AMyPlayerCharacter::JumpCompleted(const FInputActionValue& Value)
{
    bJumpHeld = false;
    bIsJumping = false;
}

void AMyPlayerCharacter::OnPrevMorphPressed()
{
    CycleMorph(-1);
}

void AMyPlayerCharacter::OnNextMorphPressed()
{
    CycleMorph(+1);
}

void AMyPlayerCharacter::BuildUnlockedMorphList(TArray<UMyMorphDataAsset*>& OutList) const
{
    OutList.Reset();

    for (UMyMorphDataAsset* Morph : AllMorphDataList)
    {
        if (!Morph) continue;
        if (Morph->UnlockTag.IsNone()) continue;

        if (UnlockedMorphTags.Contains(Morph->UnlockTag))
        {
            OutList.Add(Morph);
        }
    }
}

bool AMyPlayerCharacter::CycleMorph(int32 Direction)
{
    if (UnlockedMorphTags.Num() == 1)
    {
        return false;
    }

    if (Direction != -1 && Direction != +1)
    {
        return false;
    }

    TArray<UMyMorphDataAsset*> UnlockedMorphs;
    BuildUnlockedMorphList(UnlockedMorphs);

    if (UnlockedMorphs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CycleMorph: no unlocked morphs in AllMorphDataList."));
        return false;
    }

    // 如果当前为空或不在列表里，直接切到第一个（兜底）
    int32 CurrentIndex = UnlockedMorphs.IndexOfByKey(CurrentMorphData);
    if (CurrentIndex == INDEX_NONE)
    {
        return SwitchMorph(UnlockedMorphs[0]);
    }

    const int32 Count = UnlockedMorphs.Num();
    const int32 NextIndex = (CurrentIndex + Direction + Count) % Count;

    return SwitchMorph(UnlockedMorphs[NextIndex]);
}

void AMyPlayerCharacter::OnAbilitySlotPressed() { DispatchAbilityInputPressed(0); }
void AMyPlayerCharacter::OnAbilitySlotReleased() { DispatchAbilityInputReleased(0); }

void AMyPlayerCharacter::DispatchAbilityInputPressed(int32 SlotIndex)
{
    for (UActorComponent* Comp : ActiveMorphAbilities)
    {
        if (!IsValid(Comp)) continue;

        if (Comp->GetClass()->ImplementsInterface(UMyMorphAbilityInputReceiver::StaticClass()))
        {
            const bool bHandled = IMyMorphAbilityInputReceiver::Execute_HandleAbilityInputPressed(Comp, SlotIndex);
            if (bHandled)
            {
                return; // 只让第一个组件处理；想多播就改成 continue
            }
        }
    }
}

void AMyPlayerCharacter::DispatchAbilityInputReleased(int32 SlotIndex)
{
    for (UActorComponent* Comp : ActiveMorphAbilities)
    {
        if (!IsValid(Comp)) continue;

        if (Comp->GetClass()->ImplementsInterface(UMyMorphAbilityInputReceiver::StaticClass()))
        {
            const bool bHandled = IMyMorphAbilityInputReceiver::Execute_HandleAbilityInputReleased(Comp, SlotIndex);
            if (bHandled)
            {
                return;
            }
        }
    }
}

void AMyPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC) return;

    if (MoveAction)
    {
        EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyPlayerCharacter::Move);
    }

    if (JumpAction)
    {
        EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyPlayerCharacter::JumpStarted);
        EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyPlayerCharacter::JumpCompleted);
        EIC->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AMyPlayerCharacter::JumpCompleted);
    }

    if (InteractAction) {
        EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AMyPlayerCharacter::Interact);
    }

    if (PrevMorphAction)
    {
        EIC->BindAction(PrevMorphAction, ETriggerEvent::Started, this, &AMyPlayerCharacter::OnPrevMorphPressed);
    }

    if (NextMorphAction)
    {
        EIC->BindAction(NextMorphAction, ETriggerEvent::Started, this, &AMyPlayerCharacter::OnNextMorphPressed);
    }

    if (AbilitySlotAction)
    {
        EIC->BindAction(AbilitySlotAction, ETriggerEvent::Started, this, &AMyPlayerCharacter::OnAbilitySlotPressed);
        EIC->BindAction(AbilitySlotAction, ETriggerEvent::Completed, this, &AMyPlayerCharacter::OnAbilitySlotReleased);
        EIC->BindAction(AbilitySlotAction, ETriggerEvent::Canceled, this, &AMyPlayerCharacter::OnAbilitySlotReleased);
    }
}


void AMyPlayerCharacter::Interact() 
{
    if (CurrentInteractable && CurrentInteractable->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))  
    {
        HideInteractPrompt();
        IInteractInterface::Execute_Interact(CurrentInteractable, this);
    }
    

}


void AMyPlayerCharacter::ShowInteractPrompt()
{
    if (!InteractPromptWidget && InteractPromptClass)
    {
        InteractPromptWidget = CreateWidget<UUserWidget>(GetWorld(), InteractPromptClass);
        InteractPromptWidget->AddToViewport();

        UE_LOG(LogTemp, Warning, TEXT("Create F"));
    }
}

void AMyPlayerCharacter::HideInteractPrompt()
{
    if (InteractPromptWidget)
    {
        InteractPromptWidget->RemoveFromParent();
        UE_LOG(LogTemp, Warning, TEXT("Remove F"));
        InteractPromptWidget = nullptr;
    }
}

void AMyPlayerCharacter::SetCurrentInteractable(AActor* NewInteractable)
{
    if (NewInteractable)
    {
        CurrentInteractable = NewInteractable;
        ShowInteractPrompt();
    }
    else if (CurrentInteractable)
    {
        CurrentInteractable = nullptr;
        HideInteractPrompt();
    }
}

void AMyPlayerCharacter::StartDialogue(const TArray<FString>& Lines) 
{

    if (!DialogueWidget && DialogueWidgetClass)
    {
        DialogueWidget = CreateWidget<UDialogueWidget>(GetWorld(), DialogueWidgetClass);
        DialogueWidget->AddToViewport();

        DialogueWidget->InitDialogue(Lines);
        DialogueWidget->SetPlayerRef(this);
    }

    APlayerController* PC = Cast<APlayerController>(GetController());

    if (PC)
    {
        FInputModeGameAndUI Mode;

        Mode.SetWidgetToFocus(DialogueWidget->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
}

void AMyPlayerCharacter::EndDialogue()
{
    if (DialogueWidget)
    {
        DialogueWidget->RemoveFromParent();
        DialogueWidget = nullptr;
    }

    // 恢复提示UI（如果还在范围内）
    if (CurrentInteractable)
    {
        ShowInteractPrompt();
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}




// --- 新增函数实现 ---

bool AMyPlayerCharacter::SwitchMorph(UMyMorphDataAsset* NewMorphData) // 如果形态已解锁则切换网格、能力、键位
{
    if (!NewMorphData) return false;

    if (!UnlockedMorphTags.Contains(NewMorphData->UnlockTag))
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
                FString::Printf(TEXT("形态 '%s' 未解锁！"), *NewMorphData->GetName()));
        }
        return false;
    }

    ApplyMorphSettings(NewMorphData);
    RefreshMorphAbilities(NewMorphData);
    RefreshMorphInputContext(NewMorphData);

    CurrentMorphData = NewMorphData;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
            FString::Printf(TEXT("已切换至形态: %s"), *NewMorphData->GetName()));
    }

    return true;
}

void AMyPlayerCharacter::UnlockMorphByTag(FName Tag)
{
    if (Tag.IsNone()) return; // 防止空标签进入解锁列表

    if (!UnlockedMorphTags.Contains(Tag))
    {
        UnlockedMorphTags.Add(Tag);
    }
}

void AMyPlayerCharacter::ApplyMorphSettings(UMyMorphDataAsset* Data)
{
    if (!Data) return;

    auto* MovementComp = GetCharacterMovement();
    if (MovementComp)
    {
        MovementComp->MaxWalkSpeed = Data->MaxWalkSpeed;
        MovementComp->AirControl = Data->AirControl;
        MovementComp->GravityScale = Data->GravityScale;
    }

    // 更新临时变量，供 Tick 使用
    CurrentMaxJumpHeight = Data->MaxJumpHeight;
    CurrentJumpSpeed = Data->JumpSpeed;

    // 异步加载并设置网格和材质
    if (!Data->SkeletalMesh.IsNull()) {
        USkeletalMesh* LoadedMesh = Data->SkeletalMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            GetMesh()->SetSkeletalMeshAsset(LoadedMesh);
        }
    }

    if (!Data->Material.IsNull())
    {
        UMaterialInterface* LoadedMat = Data->Material.LoadSynchronous();
        if (LoadedMat)
        {
            GetMesh()->SetMaterial(0, LoadedMat);
        }
    }
}

void AMyPlayerCharacter::RefreshMorphInputContext(UMyMorphDataAsset* Data) // 刷新形态输入
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    ULocalPlayer* LP = PC->GetLocalPlayer();
    if (!LP) return;

    UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!Subsystem) return;

    // 移除旧的形态输入
    if (ActiveMorphMappingContext)
    {
        Subsystem->RemoveMappingContext(ActiveMorphMappingContext);
        ActiveMorphMappingContext = nullptr;
    }

    // 添加新的形态输入
    if (Data && Data->MorphMappingContext)
    {
        Subsystem->AddMappingContext(Data->MorphMappingContext, Data->MorphMappingPriority);
        ActiveMorphMappingContext = Data->MorphMappingContext;
    }

}
void AMyPlayerCharacter::RefreshMorphAbilities(UMyMorphDataAsset* Data)
{
    if (!Data) return;

    // 1. 销毁旧的组件
    for (UActorComponent* OldComp : ActiveMorphAbilities)
    {
        if (OldComp && OldComp->GetOwner() == this)
        {
            OldComp->DestroyComponent();
        }
    }
    ActiveMorphAbilities.Empty();

    // 2. 创建新的组件
    for (const TSubclassOf<UActorComponent>& CompClass : Data->AbilityComponents)
    {
        if (CompClass)
        {
            UActorComponent* NewComp = NewObject<UActorComponent>(this, CompClass, MakeUniqueObjectName(this, CompClass));
            if (NewComp)
            {
                NewComp->RegisterComponent();
                ActiveMorphAbilities.Add(NewComp);
            }
        }
    }
}