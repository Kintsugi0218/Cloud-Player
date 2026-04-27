
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SphereComponent.h"
#include "InteractInterface.h"
#include "MyNPC.generated.h"


class AMyPlayerCharacter;

UCLASS()
class CLOUD_TEST_API AMyNPC : public ACharacter, public IInteractInterface
{
	GENERATED_BODY()

public:
	AMyNPC();

protected:

	virtual void BeginPlay() override;

public:	

	virtual void Tick(float DeltaTime) override;

	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    virtual void Interact_Implementation(AActor* Interactor) override;


    // 交互触发范围
    UPROPERTY(VisibleAnywhere)
    USphereComponent* InteractionSphere;

    UPROPERTY()
    AMyPlayerCharacter* CurrentOverlappingPlayer = nullptr;

    UPROPERTY()
    FRotator TargetRotation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact | Dialogue")
    TArray<FString> DialogueLines;


    // 玩家进入
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    // 玩家离开
    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

};
