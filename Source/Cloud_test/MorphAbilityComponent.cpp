

#include "MorphAbilityComponent.h"
#include "MyPlayerCharacter.h"

UMorphAbilityComponent::UMorphAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UMorphAbilityComponent::OnAbilityAdded_Implementation()
{
}


void UMorphAbilityComponent::OnAbilityRemoved_Implementation()
{
}



void UMorphAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedPlayer = Cast<AMyPlayerCharacter>(GetOwner());
}



void UMorphAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMorphAbilityComponent::SetAbilityEnabled(bool bEnabled)
{
	bAbilityEnabled = bEnabled;

	SetComponentTickEnabled(bEnabled);
}

