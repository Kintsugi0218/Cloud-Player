
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISaveManager.generated.h"

class UMySaveGame;

UINTERFACE(Blueprintable) // 用于未来可能会被蓝图继承
class UISaveManager : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CLOUD_TEST_API IISaveManager
{
	GENERATED_BODY()

	
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SaveData(UMySaveGame* GameData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void LoadData(UMySaveGame* GameData);
};
