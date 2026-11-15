#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaEnemy.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class USceneComponent;
class AArenaPawn;

enum class EArenaEnemyType : uint8 { Alien, Turret, Rover };

UCLASS()
class AArenaEnemy : public AActor
{
    GENERATED_BODY()
public:
    AArenaEnemy();
    virtual void Tick(float DeltaTime) override;
    void SetHealth(float Health);
    void SetType(EArenaEnemyType Type);
    float GetHealthFraction() const { return RemainingHealth / MaxHealth; }
    virtual float TakeDamage(float Amount, const FDamageEvent& Event,
        AController* InstigatorController, AActor* Causer) override;
private:
    bool HasLineOfSight(const AArenaPawn& Player) const;
    void Shoot();
    void UpdateModelAnimation(float Now, bool bMoving);
    void MoveAroundCover(FVector Direction, float DeltaTime);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> Robot;
    UPROPERTY() TObjectPtr<class UAnimSequence> RobotIdle;
    UPROPERTY() TObjectPtr<class UAnimSequence> RobotMove;
    UPROPERTY() TObjectPtr<class UAnimSequence> RobotCharge;
    UPROPERTY() TObjectPtr<class UAnimSequence> RobotAttack;
    UPROPERTY() TObjectPtr<USceneComponent> Facing;
    UPROPERTY() TObjectPtr<UStaticMesh> TurretModel;
    UPROPERTY() TObjectPtr<UStaticMesh> RoverModel;
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
    float RobotAttackUntil = 0;
    FVector AttackCenter = FVector::ZeroVector;
    bool bWindingUp = false;
    FVector AttackDirection = FVector::ForwardVector;
    EArenaEnemyType Type = EArenaEnemyType::Alien;
    bool bUsesModel = false;
};
