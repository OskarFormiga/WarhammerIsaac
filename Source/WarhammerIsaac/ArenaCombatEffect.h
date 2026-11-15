#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaCombatEffect.generated.h"

enum class EArenaEffect : uint8 { Muzzle, Impact, Slide };

UCLASS()
class AArenaCombatEffect : public AActor
{
    GENERATED_BODY()

public:
    AArenaCombatEffect();
    virtual void Tick(float DeltaTime) override;
    static void Emit(UWorld& World, const FVector& Origin, const FVector& Direction,
        const FLinearColor& Color, EArenaEffect Effect);

private:
    struct FParticle
    {
        FVector Velocity;
        float Length;
        float Width;
    };

    UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Meshes;
    UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> Glow;
    TArray<FParticle, TInlineAllocator<8>> Particles;
    FLinearColor Tint;
    float Age = 0.0f;
    float Duration = 0.18f;
    float Gravity = 0.0f;
};
