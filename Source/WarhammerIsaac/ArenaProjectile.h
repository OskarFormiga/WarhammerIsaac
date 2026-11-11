#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaProjectile.generated.h"

class UProjectileMovementComponent;

UCLASS()
class AArenaProjectile : public AActor
{
    GENERATED_BODY()

public:
    AArenaProjectile();
    void Launch(float Speed, float Damage, float Lifetime, bool bHostile = false);

private:
    UFUNCTION()
    void OnImpact(const FHitResult& Hit);

    UPROPERTY()
    TObjectPtr<UProjectileMovementComponent> Movement;

    float ImpactDamage = 0.0f;
};
