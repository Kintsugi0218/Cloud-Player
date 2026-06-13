#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyCameraRigActor.h"
#include "InteractInterface.h"
#include "MyCameraVolume.generated.h"

class UBoxComponent;
class ACameraActor;

UCLASS()
class CLOUD_TEST_API AMyCameraVolume : public AActor,public IInteractInterface
{
	GENERATED_BODY()

public:
	AMyCameraVolume();

public:
	virtual void BeginPlay() override;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() override;

    // Box Collision
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    class UBoxComponent* BoxTrigger;

    // 要切换到的摄像机
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    ACameraActor* SpecialCamera;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractText")
    FText InteractText;

    UPROPERTY()
    bool bIsViewing = false;


    // 玩家进入/离开事件
    UFUNCTION()
    void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp,
        class AActor* OtherActor,
        class UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp,
        class AActor* OtherActor,
        class UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);
};