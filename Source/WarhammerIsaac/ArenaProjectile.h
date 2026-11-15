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
    virtual void Tick(float DeltaTime) override;
    void Launch(float Speed, float Damage, float Lifetime, bool bHostile = false, bool bSpreadShot = false);
    void SetPlayerTracer(const FVector& Muzzle, float Visibility);

private:
    UFUNCTION()
    void OnImpact(const FHitResult& Hit);

    UPROPERTY()
    TObjectPtr<UProjectileMovementComponent> Movement;

    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Core;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> Tracer;
    UPROPERTY() TObjectPtr<class UMaterialInterface> EmissiveMaterial;
    FLinearColor ShotColor = FLinearColor::White;
    FVector LaunchPoint = FVector::ZeroVector;
    FVector VisualOffset = FVector::ZeroVector;
    float TracerLength = 0.0f;
    float TracerWidth = 0.0f;
    bool bShowImpact = true;
    float ImpactDamage = 0.0f;
};
