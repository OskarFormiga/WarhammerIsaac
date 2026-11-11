#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaEnemy.generated.h"

class UStaticMeshComponent;
class AArenaPawn;

UCLASS()
class AArenaEnemy : public AActor
{
    GENERATED_BODY()
public:
    AArenaEnemy();
    virtual void Tick(float DeltaTime) override;
    void SetHealth(float Health);
    void SetRanged() { bRanged = true; }
    float GetHealthFraction() const { return RemainingHealth / MaxHealth; }
    virtual float TakeDamage(float Amount, const FDamageEvent& Event,
        AController* InstigatorController, AActor* Causer) override;
private:
    bool HasLineOfSight(const AArenaPawn& Player) const;
    void Shoot(const AArenaPawn& Player);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(EditDefaultsOnly, Category="Combat")
    float MoveSpeed = 155;
    UPROPERTY(EditDefaultsOnly, Category="Combat")
    float AttackDamage = 20;
    UPROPERTY(EditDefaultsOnly, Category="Combat")
    float AttackRadius = 145;
    UPROPERTY(EditDefaultsOnly, Category="Combat")
    float WindupDuration = 0.7f;
    float MaxHealth = 60;
    float RemainingHealth = 60;
    float AttackAt = 0;
    float NextAttackTime = 0;
    float HitFlashUntil = 0;
    FVector AttackCenter = FVector::ZeroVector;
    bool bWindingUp = false;
    bool bRanged = false;
};
