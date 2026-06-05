
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "ISaveManager.h"
#include "PushableActor.generated.h"

class UMySaveGame;

UCLASS()
class CLOUD_TEST_API APushableActor : public AActor, public IISaveManager
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
    TObjectPtr<UBoxComponent> InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UBoxComponent* ActorCollision;

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


public:

    bool CanBePushed() const;

    void BeginCarry(class AMyPlayerCharacter* Player);

    void EndCarry();

    virtual void SaveData_Implementation(UMySaveGame* GameData) override;
    virtual void LoadData_Implementation(UMySaveGame* GameData) override;
};
