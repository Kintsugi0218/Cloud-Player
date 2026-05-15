
#include "PushableActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MyPlayerCharacter.h"


APushableActor::APushableActor()
{
	PrimaryActorTick.bCanEverTick = true;


	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	PushPoint = CreateDefaultSubobject<USceneComponent>(TEXT("PushPoint"));
	PushPoint->SetupAttachment(Root);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(Root);

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
	return !bBeingPushed && bCanBePushed;
}

void APushableActor::BeginPush(AMyPlayerCharacter* Player)
{
	bBeingPushed = true;
	CurrentPusher = Player;
}


void APushableActor::EndPush()
{
	bBeingPushed = false;
	CurrentPusher = nullptr;
}


