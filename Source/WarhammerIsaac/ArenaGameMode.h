#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "ArenaGameMode.generated.h"

class AArenaEnemy;
class AArenaPawn;
class APointLight;

UCLASS()
class AArenaHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};

UCLASS()
class AArenaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArenaGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
    virtual void Tick(float DeltaTime) override;
    void ResetArena();
    bool IsRewardAvailable() const;
    void ChooseReward(AArenaPawn& Player, bool bDamage);
    int32 EnemiesRemaining() const;
    int32 GetRoomNumber() const { return ActiveRoom + 1; }
    static constexpr int32 RoomCount = 3;
    static FVector CameraRoomCenter(const FVector& Position);
    static bool GetPassageCameraTarget(const FVector& Position, FVector& Target);

private:
    void SpawnEncounter();
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="1"))
    float EnemyHealth = 60.0f;

    TArray<TWeakObjectPtr<AArenaEnemy>> Enemies;
    struct FExit
    {
        TWeakObjectPtr<AActor> Gate;
        TWeakObjectPtr<APointLight> Light;
        TArray<TWeakObjectPtr<AActor>> Markers;
        float OpeningTime = -1.0f;
    };
    TArray<FExit> Exits;
    int32 ActiveRoom = 0;
    bool bRewardChosen = false;
};
