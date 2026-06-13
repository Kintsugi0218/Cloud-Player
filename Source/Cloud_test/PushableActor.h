
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "ISaveManager.h"
#include "InteractInterface.h"
#include "PushableActor.generated.h"

class UMySaveGame;

UCLASS()
class CLOUD_TEST_API APushableActor : public AActor, public IISaveManager,public IInteractInterface
{
	GENERATED_BODY()
	
public:	
	
	APushableActor();

protected:
	
	virtual void BeginPlay() override;

public:	
	
	virtual void Tick(float DeltaTime) override;


    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Mesh;

    // 检测区域
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USphereComponent> InteractionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UCapsuleComponent* ActorCollision;

    UPROPERTY(EditAnywhere,BlueprintReadOnly)
    FVector CarryOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float OffsetDistance;

    // 是否正在被推动
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bBeingCarried = false;

    // 当前推动者
    UPROPERTY()
    TObjectPtr<class AMyPlayerCharacter> CurrentCarrier;

    // 当前是否可以被推动(推到指定位置之后就不允许挪动，除非需要再次利用）
    UPROPERTY(EditAnywhere,BlueprintReadWrite)
    bool bCanBePushed = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PushActorID;


    UFUNCTION()
    void OnSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

public:

    bool CanBePushed() const;

    void BeginCarry(class AMyPlayerCharacter* Player);

    void EndCarry();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() override;

    virtual void SaveData_Implementation(UMySaveGame* GameData) override;
    virtual void LoadData_Implementation(UMySaveGame* GameData) override;
};
