
#include "PushableActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyPlayerCharacter.h"
#include "MySaveGame.h"


APushableActor::APushableActor()
{
	PrimaryActorTick.bCanEverTick = true;


	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Root);
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);

	ActorCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ActorCollision"));
	ActorCollision->SetupAttachment(Root);
	ActorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ActorCollision->SetCollisionResponseToAllChannels(ECR_Block);

	Mesh->SetSimulatePhysics(false);
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



