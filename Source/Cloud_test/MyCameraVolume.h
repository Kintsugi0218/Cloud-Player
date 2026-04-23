#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyCameraRigActor.h"
#include "MyCameraVolume.generated.h"

class UBoxComponent;

UCLASS()
class CLOUD_TEST_API AMyCameraVolume : public AActor
{
	GENERATED_BODY()

public:
	AMyCameraVolume();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditAnywhere, Category = "CameraVolume")
	FMyCameraProfile VolumeProfile;

	UPROPERTY(EditAnywhere, Category = "CameraVolume")
	float BlendTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "CameraVolume")
	bool bResetToDefaultOnExit = true;
};