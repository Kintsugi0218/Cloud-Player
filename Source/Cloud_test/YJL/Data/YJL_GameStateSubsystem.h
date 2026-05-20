// YJL_GameStateSubsystem.h
// 对应 Godot game_state.gd autoload。维护已解锁形态列表 + 金币。
// 用 UGameInstanceSubsystem，零关卡依赖。
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YJL_GameStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLOnFormUnlocked, FName, FormId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYJLOnCoinChanged, int32, NewCount);

UCLASS()
class CLOUD_TEST_API UYJL_GameStateSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="YJL|GameState")
    void UnlockForm(FName FormId);

    UFUNCTION(BlueprintCallable, Category="YJL|GameState")
    bool IsFormUnlocked(FName FormId) const;

    UFUNCTION(BlueprintCallable, Category="YJL|GameState")
    const TArray<FName>& GetUnlockedForms() const { return UnlockedForms; }

    UFUNCTION(BlueprintCallable, Category="YJL|GameState")
    void AddCoin(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category="YJL|GameState")
    int32 GetCoinCount() const { return CoinCount; }

    UPROPERTY(BlueprintAssignable, Category="YJL|GameState")
    FYJLOnFormUnlocked OnFormUnlocked;

    UPROPERTY(BlueprintAssignable, Category="YJL|GameState")
    FYJLOnCoinChanged OnCoinChanged;

private:
    UPROPERTY()
    TArray<FName> UnlockedForms;

    UPROPERTY()
    int32 CoinCount = 0;
};
