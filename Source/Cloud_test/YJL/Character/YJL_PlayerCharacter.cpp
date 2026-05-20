// YJL_PlayerCharacter.cpp
#include "YJL_PlayerCharacter.h"

#include "../YJL_Common.h"
#include "../Components/YJL_FormManagerComponent.h"
#include "../Components/YJL_CarryComponent.h"
#include "../Data/YJL_FormDefinition.h"
#include "../Dialogue/YJL_DialogueSubsystem.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "Engine/World.h"
#include "Engine/Engine.h"

AYJL_PlayerCharacter::AYJL_PlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // CapsuleComponent 已由 ACharacter 创建
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 92.0f);

    // === 可见身体（StaticMesh 占位，让第三人称看得到主角）===
    // UE 自带 /Engine/BasicShapes/Cylinder.Cylinder 的 pivot 在【中心】，原始尺寸 100×100×100cm。
    // Capsule 半高 92cm（全高 184cm），所以 Z 缩放 1.84 → 圆柱全高 184cm，本地 Z 范围 [-92, +92]，
    // 正好与 Capsule 对齐。RelativeLocation 必须为 (0,0,0)，圆柱中心与 Capsule 中心重合，
    // 否则下半身会戳进地里。
    VisualBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YJL_VisualBody"));
    VisualBody->SetupAttachment(RootComponent);
    VisualBody->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    VisualBody->SetRelativeScale3D(FVector(0.84f, 0.84f, 1.84f)); // 半径 ~42cm, 高 ~184cm = Capsule 全高
    VisualBody->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 碰撞由 Capsule 处理，模型只负责显示

    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        if (CylinderFinder.Succeeded())
        {
            VisualBody->SetStaticMesh(CylinderFinder.Object);
        }
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (MatFinder.Succeeded())
        {
            VisualBody->SetMaterial(0, MatFinder.Object);
        }
    }

    // 弹簧臂
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("YJL_SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 450.0f;     // 拉远一点看清整个主角
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bEnableCameraLag = true;
    SpringArm->CameraLagSpeed = 15.0f;
    SpringArm->SocketOffset = FVector(0.0f, 50.0f, 80.0f); // 经典 TPS 越肩视角：右偏 50，上抬 80

    // 相机
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("YJL_Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    // CharacterMovement 配置
    if (auto* Move = GetCharacterMovement())
    {
        Move->bOrientRotationToMovement = true;
        Move->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
        Move->JumpZVelocity = 600.0f;       // 默认值，会被形态覆盖
        Move->AirControl = 0.8f;
        Move->MaxWalkSpeed = 600.0f;
        Move->GravityScale = 1.0f;
        Move->BrakingFrictionFactor = 1.0f;
        Move->BrakingDecelerationWalking = 5000.0f;
    }

    // 角色自己不跟随控制器的 yaw 旋转（由 OrientRotationToMovement 驱动模型转向）
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 形态管理 / 抓取
    FormManager = CreateDefaultSubobject<UYJL_FormManagerComponent>(TEXT("YJL_FormManager"));
    CarryComp = CreateDefaultSubobject<UYJL_CarryComponent>(TEXT("YJL_CarryComp"));

    FormSelectorWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("YJL_FormSelectorWidgetComp"));
    FormSelectorWidgetComp->SetupAttachment(RootComponent);
    FormSelectorWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f)); // 放置在角色头顶上方
    FormSelectorWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 关键：投射到屏幕空间，保持像素清晰与自动对齐
    FormSelectorWidgetComp->SetDrawSize(FVector2D(250.0f, 100.0f)); // 默认渲染大小
    FormSelectorWidgetComp->SetGenerateOverlapEvents(false);
}

void AYJL_PlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    CachedPC = Cast<APlayerController>(GetController());
    EnsureGameInputMode();
    RegisterIMC();

    YJL_SCREEN(3.0f, FColor::Green, "[YJL] PlayerCharacter BeginPlay 完成");
}

// 被 Bootstrap 调用：Possess 之后强制切换 InputMode（BeginPlay 时机太早会失效）
void AYJL_PlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    CachedPC = Cast<APlayerController>(NewController);
    EnsureGameInputMode();
    RegisterIMC();
}

void AYJL_PlayerCharacter::EnsureGameInputMode()
{
    if (!CachedPC)
    {
        CachedPC = Cast<APlayerController>(GetController());
    }
    if (!CachedPC) return;

    CachedPC->bShowMouseCursor = false;
    CachedPC->bEnableClickEvents = false;
    CachedPC->bEnableMouseOverEvents = false;
    // GameOnly 模式 + 鼠标锁定到窗口、隐藏鼠标 —— 这是让鼠标移动能驱动相机的关键
    FInputModeGameOnly Mode;
    Mode.SetConsumeCaptureMouseDown(true);
    CachedPC->SetInputMode(Mode);
    // 强制把鼠标焦点交给视口
    CachedPC->SetIgnoreLookInput(false);
    CachedPC->SetIgnoreMoveInput(false);
}

void AYJL_PlayerCharacter::RegisterIMC()
{
    if (!CachedPC) return;
    if (!DefaultIMC)
    {
        YJL_SCREEN(8.0f, FColor::Red, "[YJL] DefaultIMC 未挂！请在 BP_YJL_Player 的 Class Defaults → YJL|Input 里设置 IMC_YJL_Default");
        return;
    }
    if (auto* Subsys = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CachedPC->GetLocalPlayer()))
    {
        Subsys->ClearAllMappings();
        Subsys->AddMappingContext(DefaultIMC, 0);
    }
}

void AYJL_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (!CachedPC)
    {
        CachedPC = Cast<APlayerController>(GetController());
    }
    RegisterIMC();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // 防御：BP 里有任何 IA 没挂上都会在屏幕上打红字提醒
        auto BindOrWarn = [&](UInputAction* IA, const TCHAR* Name, ETriggerEvent Evt, auto Func)
        {
            if (!IA)
            {
                YJL_SCREEN(8.0f, FColor::Red, "[YJL] %s 未挂！请在 BP_YJL_Player → YJL|Input 设置", Name);
                return;
            }
            EIC->BindAction(IA, Evt, this, Func);
        };

        BindOrWarn(IA_Move,     TEXT("IA_Move"),     ETriggerEvent::Triggered, &AYJL_PlayerCharacter::OnMove);
        BindOrWarn(IA_Look,     TEXT("IA_Look"),     ETriggerEvent::Triggered, &AYJL_PlayerCharacter::OnLook);
        BindOrWarn(IA_Jump,     TEXT("IA_Jump"),     ETriggerEvent::Started,   &AYJL_PlayerCharacter::OnJumpPressed);
        BindOrWarn(IA_Interact, TEXT("IA_Interact"), ETriggerEvent::Started,   &AYJL_PlayerCharacter::OnInteractPressed);
        BindOrWarn(IA_FormPrev, TEXT("IA_FormPrev"), ETriggerEvent::Started,   &AYJL_PlayerCharacter::OnFormPrevPressed);
        BindOrWarn(IA_FormNext, TEXT("IA_FormNext"), ETriggerEvent::Started,   &AYJL_PlayerCharacter::OnFormNextPressed);
    }
}

bool AYJL_PlayerCharacter::IsDialogueActive() const
{
    if (!GetGameInstance()) return false;
    UYJL_DialogueSubsystem* DS = GetGameInstance()->GetSubsystem<UYJL_DialogueSubsystem>();
    return DS != nullptr && DS->IsActive();
}

void AYJL_PlayerCharacter::OnMove(const FInputActionValue& Value)
{
    if (IsDialogueActive()) return;
    if (!Controller) return;

    const FVector2D Axis = Value.Get<FVector2D>();
    if (Axis.IsNearlyZero()) return;

    // 取控制器 Yaw 作为相机朝向（去除 Pitch/Roll，避免朝地上看时移动变慢）
    const FRotator YawRot(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

    // Axis.X = Forward 分量（W=+1, S=-1），Axis.Y = Right 分量（D=+1, A=-1）
    AddMovementInput(Forward, Axis.X);
    AddMovementInput(Right, Axis.Y);
}

void AYJL_PlayerCharacter::OnLook(const FInputActionValue& Value)
{
    if (IsDialogueActive()) return;
    if (!Controller) return;

    const FVector2D Delta = Value.Get<FVector2D>() * MouseSensitivity;

    // 调试：屏幕角落显示当前鼠标 delta，便于确认输入到底有没有进来
    if (GEngine && !Delta.IsNearlyZero())
    {
        GEngine->AddOnScreenDebugMessage(900200, 0.2f, FColor::Magenta,
            FString::Printf(TEXT("[YJL] Look Δ=(%.2f, %.2f)"), Delta.X, Delta.Y));
    }

    APlayerController* PC = Cast<APlayerController>(Controller);
    if (!PC) return;

    // 直接改 ControlRotation —— 比 AddControllerYawInput 更直接，避免被 InputComponent 缩放
    FRotator R = PC->GetControlRotation();
    R.Yaw += Delta.X;
    // 默认：鼠标上推 = 抬头；勾上 bInvertMouseY 则反向（在 BP 的 Details 里调）
    R.Pitch += bInvertMouseY ? -Delta.Y : Delta.Y;
    R.Pitch = FMath::ClampAngle(FRotator::NormalizeAxis(R.Pitch), PitchMinDegrees, PitchMaxDegrees);
    R.Roll = 0.0f;
    PC->SetControlRotation(R);
}

void AYJL_PlayerCharacter::OnJumpPressed(const FInputActionValue& /*Value*/)
{
    if (IsDialogueActive()) return;

    const FYJLFormDefinition& Form = FormManager ? FormManager->GetCurrentForm() : FYJLFormDefinition();
    if (!Form.bCanJump)
    {
        // 当前形态不能跳 → 触发特殊能力（小熊 grab_drop）
        if (FormManager) FormManager->TrySpecialAction();
        return;
    }
    // 启用 jump buffer
    JumpBufferTimer = JumpBufferDuration;
}

void AYJL_PlayerCharacter::OnInteractPressed(const FInputActionValue& /*Value*/)
{
    // F 键：对话推进 / NPC 交互。
    // 对话激活时由 DialogueSubsystem 处理推进（它自己监听全局事件）；这里只做触发"开始对话"。
    // NPC 自己监听 OnInteractPressed 不方便（要全局事件），改成：NPC 在 Tick 检测玩家是否在范围 + Player 是否按 F。
    // 为了让 NPC 知道玩家"刚按下 F"，我们把按下事件广播给全局对话系统（它再分发或忽略）。
    if (IsDialogueActive())
    {
        if (UYJL_DialogueSubsystem* DS = GetGameInstance()->GetSubsystem<UYJL_DialogueSubsystem>())
        {
            DS->Advance();
        }
        return;
    }
    // 非对话状态：广播给 NPC 监听者
    if (UYJL_DialogueSubsystem* DS = GetGameInstance()->GetSubsystem<UYJL_DialogueSubsystem>())
    {
        DS->BroadcastInteractPressed(this);
    }
}

void AYJL_PlayerCharacter::OnFormPrevPressed(const FInputActionValue& /*Value*/)
{
    if (IsDialogueActive()) return;
    if (FormManager) FormManager->PickPrev();
}

void AYJL_PlayerCharacter::OnFormNextPressed(const FInputActionValue& /*Value*/)
{
    if (IsDialogueActive()) return;
    if (FormManager) FormManager->PickNext();
}

void AYJL_PlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 对话中：清空速度
    if (IsDialogueActive())
    {
        if (auto* Move = GetCharacterMovement())
        {
            Move->Velocity.X = 0.0f;
            Move->Velocity.Y = 0.0f;
        }
        return;
    }

    // ===== 跳跃辅助：coyote / buffer =====
    const FYJLFormDefinition& Form = FormManager ? FormManager->GetCurrentForm() : FYJLFormDefinition();

    auto* Move = GetCharacterMovement();
    if (Move)
    {
        // 独立重力：上升期用 JumpGravity，下落期用 FallGravity
        // UE 中 GravityZ = WorldGravityZ(默认 -980) * GravityScale * CustomGravityScale
        // 我们直接用 GravityScale = OurGravity / 980
        const float UEGravity = FMath::Abs(GetWorld()->GetGravityZ()); // 通常 980
        const float Target = (Move->Velocity.Z > 0.0f) ? Form.GetJumpGravity() : Form.GetFallGravity();
        Move->GravityScale = (UEGravity > KINDA_SMALL_NUMBER) ? (Target / UEGravity) : 1.0f;

        // 跳跃初速度也用形态指定
        Move->JumpZVelocity = Form.GetJumpZVelocity();
    }

    // 更新 coyote timer
    if (Move && Move->IsMovingOnGround())
    {
        CoyoteTimer = CoyoteTimeDuration;
    }
    else
    {
        CoyoteTimer -= DeltaSeconds;
    }

    // 更新 jump buffer timer
    if (JumpBufferTimer > 0.0f)
    {
        JumpBufferTimer -= DeltaSeconds;
    }

    // 触发跳跃
    if (Form.bCanJump && JumpBufferTimer > 0.0f && CoyoteTimer > 0.0f)
    {
        Jump();
        JumpBufferTimer = 0.0f;
        CoyoteTimer = 0.0f;
    }
}

void AYJL_PlayerCharacter::ApplyFormStats(const FYJLFormDefinition& Form)
{
    if (auto* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = Form.Speed;
        Move->MaxAcceleration = Form.Acceleration;
        Move->BrakingDecelerationWalking = Form.BrakingDeceleration;
        // 跳跃初速度 / 重力在 Tick 里动态写入，这里写一次是为了立刻生效
        Move->JumpZVelocity = Form.GetJumpZVelocity();
    }
    YJL_SCREEN(2.0f, FColor::Cyan, "[YJL] ApplyFormStats: %s speed=%.0f canJump=%d", *Form.Id.ToString(), Form.Speed, Form.bCanJump ? 1 : 0);
}

FVector AYJL_PlayerCharacter::GetFrontPoint(float DistanceCm) const
{
    return GetActorLocation() + GetActorForwardVector() * DistanceCm;
}


