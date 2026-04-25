#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyCameraRigActor.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;
class APawn;

USTRUCT(BlueprintType)
struct FMyCameraProfile
{
	GENERATED_BODY()

	// 相机臂长度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ArmLength = 800.f;

	// 相机角度（SpringArm相对旋转）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Pitch = -55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Yaw = 0.f;

	// 相机Rig高度（相对目标）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	float HeightOffset = 100.f;

	// 目标偏移（世界坐标偏移）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	FVector TargetOffset = FVector::ZeroVector;

	// ===== 屏幕空间死区（归一化坐标）=====
	// (0,0)=左上, (1,1)=右下, 常用中心(0.5,0.5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeadZone", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D DeadZoneCenter = FVector2D(0.5f, 0.5f);

	// 半尺寸：X/Y范围建议 0~0.5
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeadZone", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	FVector2D DeadZoneHalfSize = FVector2D(0.06f, 0.04f);

	// 跟随速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	float FollowSpeedXY = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Follow")
	float FollowSpeedZ = 2.0f;
};

UCLASS()
class CLOUD_TEST_API AMyCameraRigActor : public AActor
{
	GENERATED_BODY()

public:
	AMyCameraRigActor();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 设置跟随目标
	UFUNCTION(BlueprintCallable, Category = "CameraRig")
	void SetFollowTarget(APawn* NewTarget);

	// 切换到指定Profile（直接朝目标Profile插值）
	UFUNCTION(BlueprintCallable, Category = "CameraRig")
	void ApplyProfile(const FMyCameraProfile& NewProfile, float BlendTime = 1.f);

	// 回到默认Profile
	UFUNCTION(BlueprintCallable, Category = "CameraRig")
	void ResetToDefaultProfile(float BlendTime = 1.f);

	UFUNCTION(BlueprintPure, Category = "CameraRig")
	const FMyCameraProfile& GetCurrentProfile() const { return CurrentProfile; }

private:
	void UpdateProfileBlend(float DeltaTime); // 保留函数名，内部改为 toward-target
	void UpdateFollow(float DeltaTime);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, Category = "CameraRig")
	bool bAutoFindPlayerPawn = true;

	UPROPERTY(EditAnywhere, Category = "CameraRig")
	bool bAutoSetAsViewTarget = true;

	UPROPERTY(EditAnywhere, Category = "CameraRig")
	FMyCameraProfile DefaultProfile;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> FollowTarget;

	UPROPERTY(EditAnywhere, Category = "Camera|Follow")
	float XYTrackPlaneFollowSpeed = 3.f; // 2~6 之间都可试

	bool bXYTrackPlaneInited = false;
	float XYTrackPlaneZ = 0.f;

	// 当前生效Profile
	FMyCameraProfile CurrentProfile;

	// 目标Profile（进入Volume时更新）
	FMyCameraProfile TargetProfile;

	// 指数插值速度（由BlendTime换算）
	float ProfileInterpSpeed = 8.f;

	bool bProfileTargetInited = false;
};