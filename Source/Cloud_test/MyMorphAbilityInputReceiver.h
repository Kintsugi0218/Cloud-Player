#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MyMorphAbilityInputReceiver.generated.h"

UINTERFACE(BlueprintType)
class CLOUD_TEST_API UMyMorphAbilityInputReceiver : public UInterface
{
    GENERATED_BODY()
};

class CLOUD_TEST_API IMyMorphAbilityInputReceiver
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability Input")
    bool HandleAbilityInputPressed(int32 SlotIndex);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability Input")
    bool HandleAbilityInputReleased(int32 SlotIndex);
};