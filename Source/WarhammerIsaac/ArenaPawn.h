#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaPawn.generated.h"

class UCameraComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class USceneComponent;

enum class EArenaCombatAction : uint8
{
    Idle,
    Shooting,
    Reloading,
    Dashing
};

UCLASS(Config=Game)
class AArenaPawn : public APawn
{
    GENERATED_BODY()
public:
    AArenaPawn();
    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void Tick(float DeltaTime) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer) override;
    void ResetForRound();
    float GetHealth() const { return Health; }
    bool IsDead() const { return Health <= 0; }
    float DashCooldownRemaining() const;
    bool IsSliding() const { return SlideTimeRemaining > 0.0f; }
    bool IsCameraFollowing() const { return bCameraFollowing; }
    int32 GetAmmo() const { return Ammo; }
    int32 GetMagazineSize() const { return MagazineSize; }
    bool IsReloading() const { return CombatAction == EArenaCombatAction::Reloading; }
    float GetReloadProgress() const;
    void ApplyReward(bool bDamage);
    float GetDamageMultiplier() const { return DamageMultiplier; }
    float GetRateMultiplier() const { return RateMultiplier; }
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartFiring();
    void StopFiring();
    void Fire();
    void StartReload();
    void ChooseDamageReward();
    void ChooseRateReward();
    void RequestDash() { bDashRequested = true; }
    void Dash();
    void AdvanceDash(float DeltaTime);
    void RequestSlide();
    void TryStartSlide(float Now);
    void AdvanceSlide(float DeltaTime);
    void StartSprint() { bSprinting = true; }
    void StopSprint() { bSprinting = false; }
    void UpdateCharacterAnimation(float Now, float DeltaTime);
    FVector GetVisualMuzzle() const;
    bool IsDashing() const { return CombatAction == EArenaCombatAction::Dashing; }
    FVector GetMoveDirection() const;
    void ResetArena();
    void ToggleCameraBounds();
    void UpdateCamera(float DeltaTime, bool bSnap = false);
    void UpdateAim();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Barrel;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> AnimatedBody;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ShotgunPump;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MarineArmor;
    UPROPERTY() TObjectPtr<class UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> RunAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> ShootAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> DuckAnimation;
    UPROPERTY() TObjectPtr<class UAnimSequence> DeathAnimation;
    UPROPERTY() TObjectPtr<USceneComponent> AimPivot;
    UPROPERTY() TObjectPtr<USceneComponent> WeaponMount;
    UPROPERTY() TObjectPtr<UCameraComponent> Camera;
    UPROPERTY() TObjectPtr<UPointLightComponent> MuzzleLight;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="1"))
    float PelletDamage = 10.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.05"))
    float ShotsPerSecond = 2.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="1"))
    float ProjectileSpeed = 2250.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.1"))
    float ProjectileLifetime = 0.48f;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="1", ClampMax="16"))
    int32 PelletCount = 7;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0", ClampMax="45"))
    float SpreadHalfAngle = 13.0f;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="1", ClampMax="10"))
    int32 MagazineSize = 5;
    UPROPERTY(EditDefaultsOnly, Category="Combat", meta=(ClampMin="0.1"))
    float ReloadDuration = 1.35f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Movement", meta=(ClampMin="1"))
    float RunSpeed = 575.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Movement", meta=(ClampMin="1"))
    float SprintSpeed = 750.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Movement", meta=(ClampMin="0.1"))
    float SlideDuration = 0.46f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Movement", meta=(ClampMin="1"))
    float SlideStartSpeed = 1220.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Dash", meta=(ClampMin="0.01"))
    float DashDuration = 0.34f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Dash", meta=(ClampMin="0"))
    float DashInvulnerabilityDuration = 0.30f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Dash", meta=(ClampMin="1"))
    float DashSpeed = 930.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Dash", meta=(ClampMin="0.1"))
    float DashCooldown = 1.2f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Camera")
    FRotator CameraAngle = FRotator(-50, 45, 0);
    UPROPERTY(Config, EditDefaultsOnly, Category="Camera", meta=(ClampMin="200"))
    float CameraDistance = 3000.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Camera", meta=(ClampMin="20", ClampMax="80"))
    float CameraFieldOfView = 50.0f;
    UPROPERTY(Config, EditDefaultsOnly, Category="Camera")
    FVector2D CameraFollowExtent = FVector2D(600, 600);

    FVector CameraFocus = FVector::ZeroVector;
    float CameraFollowBlend = 0.0f;
    FVector LastMoveDirection = FVector::ForwardVector;
    FVector DashDirection = FVector::ForwardVector;
    FVector SlideDirection = FVector::ForwardVector;
    FVector CameraMovementLead = FVector::ZeroVector;
    float SlideTimeRemaining = 0.0f;
    float SlideBufferedUntil = 0.0f;
    float NextSlideTime = 0.0f;
    float NextSlideSpark = 0.0f;
    FVector VisualStride = FVector::ZeroVector;
    bool bSprinting = false;
    FVector2D MoveInput = FVector2D::ZeroVector;
    float Health = 100;
    float NextShotTime = 0;
    float BufferedShotUntil = 0.0f;
    int32 Ammo = 0;
    float ReloadEndsAt = 0.0f;
    float DamageMultiplier = 1.0f;
    float RateMultiplier = 1.0f;
    EArenaCombatAction CombatAction = EArenaCombatAction::Idle;
    float DashTimeRemaining = 0.0f;
    float DashInvulnerableUntil = 0.0f;
    float NextDashTime = 0;
    float InvulnerableUntil = 0;
    float HitFlashUntil = 0;
    float MuzzleFlashUntil = 0;
    bool bFiring = false;
    bool bDashRequested = false;
    float ReloadPose = 0.0f;
    bool bCameraFollowing = true;
    bool bShowCameraBounds = false;
    bool bUsesCharacterModel = false;
    bool bUsesWeaponModel = false;
};
