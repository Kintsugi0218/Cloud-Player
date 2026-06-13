#include "MyCameraVolume.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerCharacter.h"

AMyCameraVolume::AMyCameraVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxTrigger"));
	RootComponent = BoxTrigger;
	BoxTrigger->SetCollisionProfileName("Trigger");

	BoxTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMyCameraVolume::OnOverlapBegin);
	BoxTrigger->OnComponentEndOverlap.AddDynamic(this, &AMyCameraVolume::OnOverlapEnd);
}


void AMyCameraVolume::BeginPlay()
{
	Super::BeginPlay();
}

void AMyCameraVolume::Interact_Implementation(AActor* Interactor)
{
	if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(Interactor)) 
	{
		if (!bIsViewing) {
			Cast<APlayerController>(Player->GetController())->SetViewTargetWithBlend(SpecialCamera, 1.0f);
			bIsViewing = true;
			Player->HideInteractPrompt();
		}
		else
		{
			Cast<APlayerController>(Player->GetController())->SetViewTargetWithBlend(UGameplayStatics::GetPlayerPawn(this, 0), 1.0f);
			bIsViewing = false;
			Player->ShowInteractPrompt(InteractText);
		}
	}
}

FText AMyCameraVolume::GetInteractionText_Implementation()
{
	return InteractText;
}

void AMyCameraVolume::OnOverlapBegin(class UPrimitiveComponent* OverlappedComp,class AActor* OtherActor,
	class UPrimitiveComponent* OtherComp,int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult) 
{
	if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(OtherActor)) {
		Player->SetCurrentInteractable(this);
	}
}

void AMyCameraVolume::OnOverlapEnd(UPrimitiveComponent* OverlappedComp,AActor* OtherActor,
	UPrimitiveComponent* OtherComp,int32 OtherBodyIndex)
{
	if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(OtherActor)) {
		Player->SetCurrentInteractable(nullptr);
		Player->HideInteractPrompt();
	}
}

