
#include "PushableActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyPlayerCharacter.h"
#include "BearPushAbility.h"
#include "MySaveGame.h"


APushableActor::APushableActor()
{
	PrimaryActorTick.bCanEverTick = true;


	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBox"));
	InteractionSphere->SetupAttachment(Root);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

	ActorCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("ActorCollision"));
	ActorCollision->SetupAttachment(Root);
	ActorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ActorCollision->SetCollisionResponseToAllChannels(ECR_Block);

	Mesh->SetSimulatePhysics(false);

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this,&APushableActor::OnSphereBeginOverlap);

	InteractionSphere->OnComponentEndOverlap.AddDynamic(this,&APushableActor::OnSphereEndOverlap);
}


void APushableActor::BeginPlay()
{
	Super::BeginPlay();
}


void APushableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



bool APushableActor::CanBePushed() const
{
	return !bBeingCarried && bCanBePushed;
}
void APushableActor::BeginCarry(AMyPlayerCharacter* Player)
{
	bBeingCarried = true;
	CurrentCarrier = Player;
}
void APushableActor::EndCarry()
{
	bBeingCarried = false;
	CurrentCarrier = nullptr;
}

void APushableActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(OtherActor)) 
	{
		if (Player->FindComponentByClass<UBearPushAbility>()) 
		{
			Player->SetCurrentInteractable(this);
			Player->ShowInteractPrompt(GetInteractionText_Implementation());
		}
	}
}

void APushableActor::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AMyPlayerCharacter* Player = Cast<AMyPlayerCharacter>(OtherActor))
	{
		if (Player->FindComponentByClass<UBearPushAbility>() && Player->CurrentInteractable == this)
		{
			Player->SetCurrentInteractable(nullptr);
			Player->HideInteractPrompt();
		}
	}
}


void APushableActor::Interact_Implementation(AActor* Interactor)
{
}

FText APushableActor::GetInteractionText_Implementation()
{
	return FText::FromString(TEXT("°áÔË"));
}

void APushableActor::SaveData_Implementation(UMySaveGame* GameData)
{
	GameData->PushActorPositions.Add(PushActorID, GetActorLocation());
	GameData->bPushActorCanBePushed.Add(PushActorID, bCanBePushed);
}

void APushableActor::LoadData_Implementation(UMySaveGame* GameData)
{
	if (FVector* Location = GameData->PushActorPositions.Find(PushActorID)) {
		SetActorLocation(*Location);
	}

	if (bool* ActorCanBePushed = GameData->bPushActorCanBePushed.Find(PushActorID)) {
		bCanBePushed = *ActorCanBePushed;
	}
}



