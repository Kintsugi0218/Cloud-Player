#include "MyCameraRigActor.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	static FVector2D InterpVector2D(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float Speed)
	{
		return FVector2D(
			FMath::FInterpTo(Current.X, Target.X, DeltaTime, Speed),
			FMath::FInterpTo(Current.Y, Target.Y, DeltaTime, Speed));
	}
}

AMyCameraRigActor::AMyCameraRigActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Root);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->TargetArmLength = DefaultProfile.ArmLength;
	CameraBoom->SetRelativeRotation(FRotator(DefaultProfile.Pitch, DefaultProfile.Yaw, 0.f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CurrentProfile = DefaultProfile;
	TargetProfile = DefaultProfile;
}

void AMyCameraRigActor::BeginPlay()
{
	Super::BeginPlay();

	CurrentProfile = DefaultProfile;
	TargetProfile = DefaultProfile;
	bProfileTargetInited = true;

	CameraBoom->TargetArmLength = CurrentProfile.ArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CurrentProfile.Pitch, CurrentProfile.Yaw, 0.f));

	if (bAutoFindPlayerPawn)
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			SetFollowTarget(Pawn);
		}
	}

	if (bAutoSetAsViewTarget)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->SetViewTarget(this);
		}
	}
}

void AMyCameraRigActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateProfileBlend(DeltaTime);
	UpdateFollow(DeltaTime);

	CameraBoom->TargetArmLength = CurrentProfile.ArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CurrentProfile.Pitch, CurrentProfile.Yaw, 0.f));
}

void AMyCameraRigActor::SetFollowTarget(APawn* NewTarget)
{
	FollowTarget = NewTarget;
}

void AMyCameraRigActor::ApplyProfile(const FMyCameraProfile& NewProfile, float BlendTime)
{
	TargetProfile = NewProfile;
	bProfileTargetInited = true;

	if (BlendTime <= KINDA_SMALL_NUMBER)
	{
		CurrentProfile = TargetProfile;
		ProfileInterpSpeed = 9999.f;
		return;
	}

	// 经验换算：约 BlendTime 内接近到位
	ProfileInterpSpeed = 1.5f / BlendTime;
}

void AMyCameraRigActor::ResetToDefaultProfile(float BlendTime)
{
	ApplyProfile(DefaultProfile, BlendTime);
}

void AMyCameraRigActor::UpdateProfileBlend(float DeltaTime)
{
	if (!bProfileTargetInited) return;

	const float S = FMath::Max(ProfileInterpSpeed, 0.f);

	CurrentProfile.ArmLength = FMath::FInterpTo(CurrentProfile.ArmLength, TargetProfile.ArmLength, DeltaTime, S);
	CurrentProfile.Pitch = FMath::FInterpTo(CurrentProfile.Pitch, TargetProfile.Pitch, DeltaTime, S);
	CurrentProfile.Yaw = FMath::FInterpTo(CurrentProfile.Yaw, TargetProfile.Yaw, DeltaTime, S);
	CurrentProfile.HeightOffset = FMath::FInterpTo(CurrentProfile.HeightOffset, TargetProfile.HeightOffset, DeltaTime, S);

	CurrentProfile.TargetOffset = FMath::VInterpTo(CurrentProfile.TargetOffset, TargetProfile.TargetOffset, DeltaTime, S);
	CurrentProfile.DeadZoneCenter = InterpVector2D(CurrentProfile.DeadZoneCenter, TargetProfile.DeadZoneCenter, DeltaTime, S);
	CurrentProfile.DeadZoneHalfSize = InterpVector2D(CurrentProfile.DeadZoneHalfSize, TargetProfile.DeadZoneHalfSize, DeltaTime, S);

	CurrentProfile.FollowSpeedXY = FMath::FInterpTo(CurrentProfile.FollowSpeedXY, TargetProfile.FollowSpeedXY, DeltaTime, S);
	CurrentProfile.FollowSpeedZ = FMath::FInterpTo(CurrentProfile.FollowSpeedZ, TargetProfile.FollowSpeedZ, DeltaTime, S);
}

void AMyCameraRigActor::UpdateFollow(float DeltaTime)
{
	APawn* TargetPawn = FollowTarget.Get();
	if (!IsValid(TargetPawn)) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	int32 ViewW = 0;
	int32 ViewH = 0;
	PC->GetViewportSize(ViewW, ViewH);
	if (ViewW <= 0 || ViewH <= 0) return;

	const FVector TargetPoint = TargetPawn->GetActorLocation() + CurrentProfile.TargetOffset;
	const FVector CamLoc = GetActorLocation();

	const float DesiredZ = TargetPoint.Z + CurrentProfile.HeightOffset;

	if (!bXYTrackPlaneInited)
	{
		XYTrackPlaneZ = TargetPoint.Z;
		bXYTrackPlaneInited = true;
	}
	else
	{
		XYTrackPlaneZ = FMath::FInterpTo(XYTrackPlaneZ, TargetPoint.Z, DeltaTime, XYTrackPlaneFollowSpeed);
	}

	float DesiredX = CamLoc.X;
	float DesiredY = CamLoc.Y;

	FVector2D TargetScreenPx;
	if (!PC->ProjectWorldLocationToScreen(TargetPoint, TargetScreenPx, true))
	{
		FVector NewLoc = CamLoc;
		NewLoc.Z = FMath::FInterpTo(CamLoc.Z, DesiredZ, DeltaTime, CurrentProfile.FollowSpeedZ);
		SetActorLocation(NewLoc);
		return;
	}

	const FVector2D TargetNorm(
		TargetScreenPx.X / static_cast<float>(ViewW),
		TargetScreenPx.Y / static_cast<float>(ViewH));

	const FVector2D DZCenter(
		FMath::Clamp(CurrentProfile.DeadZoneCenter.X, 0.f, 1.f),
		FMath::Clamp(CurrentProfile.DeadZoneCenter.Y, 0.f, 1.f));

	const FVector2D DZHalf(
		FMath::Clamp(CurrentProfile.DeadZoneHalfSize.X, 0.f, 0.5f),
		FMath::Clamp(CurrentProfile.DeadZoneHalfSize.Y, 0.f, 0.5f));

	const FVector2D Min = DZCenter - DZHalf;
	const FVector2D Max = DZCenter + DZHalf;

	const FVector2D ClampedNorm(
		FMath::Clamp(TargetNorm.X, Min.X, Max.X),
		FMath::Clamp(TargetNorm.Y, Min.Y, Max.Y));

	if (!TargetNorm.Equals(ClampedNorm, 0.0001f))
	{
		const FVector2D ClampedScreenPx(
			ClampedNorm.X * static_cast<float>(ViewW),
			ClampedNorm.Y * static_cast<float>(ViewH));

		FVector RayO1, RayD1, RayO2, RayD2;
		const bool bOK1 = PC->DeprojectScreenPositionToWorld(TargetScreenPx.X, TargetScreenPx.Y, RayO1, RayD1);
		const bool bOK2 = PC->DeprojectScreenPositionToWorld(ClampedScreenPx.X, ClampedScreenPx.Y, RayO2, RayD2);

		if (bOK1 && bOK2)
		{
			const float PlaneZ = XYTrackPlaneZ;
			const float MinAbsRayZ = 0.08f;
			const float MaxRayT = 30000.f;

			auto IntersectAtZ = [&](const FVector& O, const FVector& D, FVector& OutHit) -> bool
				{
					if (FMath::Abs(D.Z) < MinAbsRayZ) return false;

					const float T = (PlaneZ - O.Z) / D.Z;
					if (T < 0.f || T > MaxRayT) return false;

					OutHit = O + D * T;
					return FMath::IsFinite(OutHit.X) && FMath::IsFinite(OutHit.Y) && FMath::IsFinite(OutHit.Z);
				};

			FVector HitTarget, HitClamp;
			if (IntersectAtZ(RayO1, RayD1, HitTarget) && IntersectAtZ(RayO2, RayD2, HitClamp))
			{
				FVector DeltaWorld = HitTarget - HitClamp;
				DeltaWorld.Z = 0.f;

				if (FMath::IsFinite(DeltaWorld.X) && FMath::IsFinite(DeltaWorld.Y) && FMath::IsFinite(DeltaWorld.Z))
				{
					const float MaxPushPerFrame = 80.f;
					DeltaWorld = DeltaWorld.GetClampedToMaxSize2D(MaxPushPerFrame);

					DesiredX = CamLoc.X + DeltaWorld.X;
					DesiredY = CamLoc.Y + DeltaWorld.Y;
				}
			}
		}
	}

	const float DistXY = FVector2D(DesiredX - CamLoc.X, DesiredY - CamLoc.Y).Size();
	const float DistZ = FMath::Abs(DesiredZ - CamLoc.Z);

	const float BaseSpeedXY = CurrentProfile.FollowSpeedXY;
	const float GainXYPerCm = 0.05f;
	const float MinSpeedXY = BaseSpeedXY;
	const float MaxSpeedXY = FMath::Max(18.f, BaseSpeedXY);

	const float BaseSpeedZ = CurrentProfile.FollowSpeedZ;
	const float GainZPerCm = 0.08f;
	const float MinSpeedZ = BaseSpeedZ;
	const float MaxSpeedZ = FMath::Max(20.f, BaseSpeedZ);

	const float DynamicSpeedXY = FMath::Clamp(BaseSpeedXY + DistXY * GainXYPerCm, MinSpeedXY, MaxSpeedXY);
	const float DynamicSpeedZ = FMath::Clamp(BaseSpeedZ + DistZ * GainZPerCm, MinSpeedZ, MaxSpeedZ);

	FVector NewLoc = CamLoc;
	NewLoc.X = FMath::FInterpTo(CamLoc.X, DesiredX, DeltaTime, DynamicSpeedXY);
	NewLoc.Y = FMath::FInterpTo(CamLoc.Y, DesiredY, DeltaTime, DynamicSpeedXY);
	NewLoc.Z = FMath::FInterpTo(CamLoc.Z, DesiredZ, DeltaTime, DynamicSpeedZ);

	SetActorLocation(NewLoc);
}