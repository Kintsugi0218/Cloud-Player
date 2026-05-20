// YJL_CarryableComponent.cpp
#include "YJL_CarryableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UYJL_CarryableComponent::UYJL_CarryableComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UPrimitiveComponent* UYJL_CarryableComponent::GetPhysicsRoot() const
{
    if (AActor* Owner = GetOwner())
    {
        return Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    }
    return nullptr;
}
