#include "ArenaPawn.h"
#include "ArenaGameMode.h"
#include "ArenaProjectile.h"
#include "ArenaVisuals.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FVector BarrelRestPosition(76, 0, 6);
    constexpr float FlashDuration = 0.065f;
    constexpr float RecoilDuration = 0.15f;
    constexpr float ShotBufferDuration = 0.12f;
}

AArenaPawn::AArenaPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    USphereComponent* Hitbox = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
    Hitbox->InitSphereRadius(42);
    Hitbox->SetCollisionProfileName(TEXT("Pawn"));
    SetRootComponent(Hitbox);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Hitbox);
    Body->SetRelativeScale3D(FVector(0.74f, 0.68f, 0.94f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetStaticMesh(Cube.Object);

    UStaticMeshComponent* Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Helmet"));
    Head->SetupAttachment(Body);
    Head->SetStaticMesh(Cube.Object);
    Head->SetRelativeLocation(FVector(0, 0, 69));
    Head->SetRelativeScale3D(FVector(0.60f, 0.70f, 0.45f));
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UStaticMeshComponent* Legs = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Legs"));
    Legs->SetupAttachment(Body);
    Legs->SetStaticMesh(Cube.Object);
    Legs->SetRelativeLocation(FVector(0, 0, -64));
    Legs->SetRelativeScale3D(FVector(0.70f, 0.80f, 0.48f));
    Legs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
    Barrel->SetupAttachment(Body);
    Barrel->SetStaticMesh(Cube.Object);
    Barrel->SetRelativeLocation(BarrelRestPosition);
    Barrel->SetRelativeScale3D(FVector(0.90f, 0.40f, 0.33f));
    Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
    MuzzleLight->SetupAttachment(Barrel);
    MuzzleLight->SetRelativeLocation(FVector(55, 0, 0));
    MuzzleLight->SetLightColor(FLinearColor(1, 0.62f, 0.18f));
    MuzzleLight->SetIntensity(0);
    MuzzleLight->SetAttenuationRadius(320);
    MuzzleLight->SetCastShadows(false);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
    Camera->SetupAttachment(Hitbox);
    Camera->SetAbsolute(true, true, true);
    Camera->bConstrainAspectRatio = true;
    Camera->AspectRatio = 16.0f / 9.0f;
    UFloatingPawnMovement* Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Hitbox);
    Movement->MaxSpeed = 440;
    Movement->Acceleration = 2800;
    Movement->Deceleration = 3400;
    Movement->TurningBoost = 8;
    Movement->SetPlaneConstraintEnabled(true);
    Movement->SetPlaneConstraintNormal(FVector::UpVector);
}

void AArenaPawn::BeginPlay()
{
    Super::BeginPlay();
    ResetForRound();
    ArenaVisuals::Tint(Barrel, FLinearColor(0.35f, 0.29f, 0.19f));
}

void AArenaPawn::PawnClientRestart()
{
    Super::PawnClientRestart();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = true;
        PC->CurrentMouseCursor = EMouseCursor::Crosshairs;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
    }
}

void AArenaPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("Forward"), this, &AArenaPawn::MoveForward);
    Input->BindAxis(TEXT("Right"), this, &AArenaPawn::MoveRight);
    Input->BindAction(TEXT("Fire"), IE_Pressed, this, &AArenaPawn::StartFiring);
    Input->BindAction(TEXT("Fire"), IE_Released, this, &AArenaPawn::StopFiring);
    Input->BindAction(TEXT("Dash"), IE_Pressed, this, &AArenaPawn::RequestDash);
    Input->BindAction(TEXT("Reload"), IE_Pressed, this, &AArenaPawn::StartReload);
    Input->BindAction(TEXT("DamageReward"), IE_Pressed, this, &AArenaPawn::ChooseDamageReward);
    Input->BindAction(TEXT("RateReward"), IE_Pressed, this, &AArenaPawn::ChooseRateReward);
    Input->BindAction(TEXT("ResetArena"), IE_Pressed, this, &AArenaPawn::ResetArena);
    Input->BindAction(TEXT("CameraBounds"), IE_Pressed, this, &AArenaPawn::ToggleCameraBounds);
}

void AArenaPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const float Now = GetWorld()->GetTimeSeconds();
    if (!IsDead())
    {
        if (bDashRequested)
        {
            bDashRequested = false;
            Dash();
        }
        if (IsReloading() && Now >= ReloadEndsAt)
        {
            Ammo = MagazineSize;
            ReloadEndsAt = 0.0f;
            CombatAction = EArenaCombatAction::Idle;
        }
        else if (CombatAction == EArenaCombatAction::Shooting && Now >= NextShotTime)
        {
            CombatAction = EArenaCombatAction::Idle;
        }
        if (IsDashing())
        {
            AdvanceDash(DeltaTime);
        }
        else
        {
            const FVector MoveDirection = GetMoveDirection();
            AddMovementInput(MoveDirection);
            if (!MoveDirection.IsNearlyZero())
            {
                LastMoveDirection = MoveDirection;
            }
        }
        if (Ammo == 0)
        {
            StartReload();
        }
    }

    CastChecked<USphereComponent>(GetRootComponent())->SetCollisionResponseToChannel(
        ECC_GameTraceChannel1, Now < DashInvulnerableUntil ? ECR_Ignore : ECR_Block);
    UpdateCamera(DeltaTime);
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (PC && !IsDead())
    {
        UpdateAim();
        const bool bHoldingFire = bFiring && PC->IsInputKeyDown(EKeys::LeftMouseButton);
        if (bHoldingFire || Now < BufferedShotUntil)
        {
            Fire();
        }
    }
    MuzzleLight->SetIntensity(Now < MuzzleFlashUntil ? 8500 : 0);
    const float Recoil = MuzzleFlashUntil > 0.0f
        ? FMath::Clamp((MuzzleFlashUntil - FlashDuration + RecoilDuration - Now) / RecoilDuration, 0.0f, 1.0f)
        : 0.0f;
    const float ReloadTarget = IsReloading() ? FMath::Sin(PI * GetReloadProgress()) : 0.0f;
    ReloadPose = FMath::FInterpTo(ReloadPose, ReloadTarget, DeltaTime, 22.0f);
    Barrel->SetRelativeLocation(BarrelRestPosition - FVector(30.0f * Recoil * Recoil, 0, 28.0f * ReloadPose));
    Barrel->SetRelativeRotation(FRotator(-35.0f * ReloadPose, 0, 0));
    FLinearColor Color(0.04f, 0.32f, 0.52f);
    if (IsDead())
    {
        Color = FLinearColor(0.12f, 0.12f, 0.14f);
    }
    else if (Now < HitFlashUntil)
    {
        Color = FLinearColor(1, 0.18f, 0.10f);
    }
    else if (Now < InvulnerableUntil || Now < DashInvulnerableUntil)
    {
        Color = FLinearColor(0.3f, 0.95f, 1);
    }
    ArenaVisuals::Tint(Body, Color);
    if (bShowCameraBounds)
    {
        DrawDebugBox(GetWorld(), AArenaGameMode::CameraRoomCenter(GetActorLocation()) + FVector(0, 0, 10),
            FVector(CameraFollowExtent.X, CameraFollowExtent.Y, 2),
            bCameraFollowing ? FColor::Cyan : FColor::Orange, false, -1, 0, 3);
    }
}

void AArenaPawn::UpdateCamera(float DeltaTime, bool bSnap)
{
    const FVector Position = GetActorLocation();
    const FVector RoomCenter = AArenaGameMode::CameraRoomCenter(Position);
    const FVector RoomPosition = Position - RoomCenter;
    bCameraFollowing = FMath::Abs(RoomPosition.X) <= CameraFollowExtent.X &&
        FMath::Abs(RoomPosition.Y) <= CameraFollowExtent.Y;
    FVector FollowTarget(Position.X, Position.Y, 65);
    const bool bInPassage = AArenaGameMode::GetPassageCameraTarget(Position, FollowTarget);
    if (bSnap)
    {
        CameraFocus = RoomCenter + FVector(FMath::Clamp(RoomPosition.X, -CameraFollowExtent.X, CameraFollowExtent.X),
            FMath::Clamp(RoomPosition.Y, -CameraFollowExtent.Y, CameraFollowExtent.Y), 65);
        CameraFollowBlend = 1.0f;
    }
    else if (bCameraFollowing || bInPassage)
    {
        CameraFollowBlend = FMath::Min(1.0f, CameraFollowBlend + DeltaTime / 0.45f);
        const double FollowWeight = FMath::SmoothStep(0.0f, 1.0f, CameraFollowBlend);
        const double Alpha = 1.0 - FMath::Exp(-7.0 * FollowWeight * DeltaTime);
        CameraFocus = FMath::Lerp(CameraFocus, FollowTarget, Alpha);
    }
    else
    {
        CameraFollowBlend = 0.0f;
    }
    // Keep the last framing outside the zone until the player approaches the screen edge.
    const FRotator Yaw(0, CameraAngle.Yaw, 0);
    const FVector Forward = Yaw.Vector();
    const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
    const double HalfWidth = CameraDistance * FMath::Tan(FMath::DegreesToRadians(CameraFieldOfView * 0.5));
    const double ForwardLimit = HalfWidth * 0.60 / Camera->AspectRatio;
    const double RightLimit = HalfWidth * 0.70;
    const FVector Offset = Position - CameraFocus;
    const double Along = FVector::DotProduct(Offset, Forward);
    const double Across = FVector::DotProduct(Offset, Right);
    CameraFocus += Forward * (Along - FMath::Clamp(Along, -ForwardLimit, ForwardLimit))
        + Right * (Across - FMath::Clamp(Across, -RightLimit, RightLimit));
    Camera->SetWorldLocationAndRotation(CameraFocus - CameraAngle.Vector() * CameraDistance, CameraAngle);
    Camera->SetFieldOfView(CameraFieldOfView);
}

void AArenaPawn::MoveForward(float Value) { MoveInput.X = Value; }
void AArenaPawn::MoveRight(float Value) { MoveInput.Y = Value; }
void AArenaPawn::StartFiring()
{
    bFiring = true;
    BufferedShotUntil = GetWorld()->GetTimeSeconds() + ShotBufferDuration;
}
void AArenaPawn::StopFiring() { bFiring = false; }
void AArenaPawn::ToggleCameraBounds() { bShowCameraBounds = !bShowCameraBounds; }

void AArenaPawn::UpdateAim()
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    FVector Origin;
    FVector Direction;
    if (!PC || !PC->DeprojectMousePositionToWorld(Origin, Direction) || FMath::IsNearlyZero(Direction.Z))
    {
        return;
    }
    const double Distance = (GetActorLocation().Z - Origin.Z) / Direction.Z;
    const FVector Aim = (Origin + Direction * Distance - GetActorLocation()).GetSafeNormal2D();
    if (Distance > 0 && !Aim.IsNearlyZero())
    {
        Body->SetWorldRotation(Aim.Rotation());
    }
}

void AArenaPawn::Fire()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (IsDead() || IsDashing() || Now < NextShotTime || IsReloading() || Ammo <= 0)
    {
        return;
    }
    UpdateAim();
    BufferedShotUntil = 0.0f;
    CombatAction = EArenaCombatAction::Shooting;
    NextShotTime = Now + 1.0f / FMath::Max(0.05f, ShotsPerSecond * RateMultiplier);
    --Ammo;
    FActorSpawnParameters Spawn;
    Spawn.Owner = this;
    Spawn.Instigator = this;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FRotator Aim = Body->GetComponentRotation();
    const int32 Count = FMath::Clamp(PelletCount, 1, 16);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Offset = Count > 1 ? FMath::Lerp(-SpreadHalfAngle, SpreadHalfAngle,
            static_cast<float>(Index) / (Count - 1)) : 0.0f;
        const FRotator PelletDirection(0, Aim.Yaw + Offset, 0);
        if (AArenaProjectile* Pellet = GetWorld()->SpawnActor<AArenaProjectile>(
            GetActorLocation(), PelletDirection, Spawn))
        {
            Pellet->Launch(ProjectileSpeed, PelletDamage * DamageMultiplier, ProjectileLifetime);
        }
    }
    MuzzleFlashUntil = Now + FlashDuration;
    if (Ammo == 0)
    {
        StartReload();
    }
}

void AArenaPawn::StartReload()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (!IsDead() && Ammo < MagazineSize && !IsReloading() && !IsDashing())
    {
        CombatAction = EArenaCombatAction::Reloading;
        ReloadEndsAt = Now + ReloadDuration;
    }
}

float AArenaPawn::GetReloadProgress() const
{
    return IsReloading() ? FMath::Clamp(1.0f - (ReloadEndsAt - GetWorld()->GetTimeSeconds()) / ReloadDuration,
        0.0f, 1.0f) : 0.0f;
}

void AArenaPawn::ChooseDamageReward()
{
    if (AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
    {
        Mode->ChooseReward(*this, true);
    }
}

void AArenaPawn::ChooseRateReward()
{
    if (AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
    {
        Mode->ChooseReward(*this, false);
    }
}

void AArenaPawn::ApplyReward(bool bDamage)
{
    if (bDamage)
    {
        DamageMultiplier *= 1.25f;
    }
    else
    {
        RateMultiplier *= 1.30f;
    }
}

FVector AArenaPawn::GetMoveDirection() const
{
    const FRotator Yaw(0, CameraAngle.Yaw, 0);
    return (Yaw.Vector() * MoveInput.X + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * MoveInput.Y)
        .GetClampedToMaxSize(1.0);
}

void AArenaPawn::Dash()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (IsDead() || IsDashing() || Now < NextDashTime)
    {
        return;
    }
    DashDirection = GetMoveDirection().GetSafeNormal();
    if (DashDirection.IsNearlyZero())
    {
        DashDirection = LastMoveDirection;
    }
    GetMovementComponent()->StopMovementImmediately();
    ConsumeMovementInputVector();
    CombatAction = EArenaCombatAction::Dashing;
    DashTimeRemaining = FMath::Max(0.01f, DashDuration);
    ReloadEndsAt = 0.0f;
    NextDashTime = Now + DashCooldown;
    DashInvulnerableUntil = Now + FMath::Max(0.0f, DashInvulnerabilityDuration);
    CastChecked<USphereComponent>(GetRootComponent())->SetCollisionResponseToChannel(
        ECC_GameTraceChannel1, Now < DashInvulnerableUntil ? ECR_Ignore : ECR_Block);
}

void AArenaPawn::AdvanceDash(float DeltaTime)
{
    const float Duration = FMath::Max(0.01f, DashDuration);
    const float Start = 1.0f - DashTimeRemaining / Duration;
    DashTimeRemaining = FMath::Max(0.0f, DashTimeRemaining - DeltaTime);
    const float End = 1.0f - DashTimeRemaining / Duration;
    // A small ease-out preserves the total travel while easing back into walking.
    const float DistanceFraction = (End - Start) * (1.0f + 0.35f * (1.0f - Start - End));
    FHitResult Hit;
    AddActorWorldOffset(DashDirection * (DashSpeed * Duration * DistanceFraction), true, &Hit);
    if (Hit.bBlockingHit || DashTimeRemaining <= 0.0f)
    {
        CombatAction = EArenaCombatAction::Idle;
        DashTimeRemaining = 0.0f;
    }
}

float AArenaPawn::TakeDamage(float Amount, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer)
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (IsDead() || Now < InvulnerableUntil || Now < DashInvulnerableUntil)
    {
        return 0;
    }
    const float Applied = FMath::Clamp(Amount, 0.0f, Health);
    if (Applied <= 0)
    {
        return 0;
    }
    Health -= Applied;
    HitFlashUntil = Now + 0.16f;
    InvulnerableUntil = Now + 0.55f;
    if (IsDead())
    {
        bFiring = false;
        ReloadEndsAt = 0.0f;
        CombatAction = EArenaCombatAction::Idle;
        DashTimeRemaining = DashInvulnerableUntil = BufferedShotUntil = 0.0f;
        bDashRequested = false;
        GetMovementComponent()->StopMovementImmediately();
        ConsumeMovementInputVector();
    }
    return Applied;
}

float AArenaPawn::DashCooldownRemaining() const
{
    return FMath::Max(0.0f, NextDashTime - GetWorld()->GetTimeSeconds());
}

void AArenaPawn::ResetForRound()
{
    GetMovementComponent()->StopMovementImmediately();
    ConsumeMovementInputVector();
    Health = 100;
    Ammo = MagazineSize;
    ReloadEndsAt = 0.0f;
    DamageMultiplier = RateMultiplier = 1.0f;
    CombatAction = EArenaCombatAction::Idle;
    DashTimeRemaining = DashInvulnerableUntil = BufferedShotUntil = ReloadPose = 0.0f;
    NextShotTime = NextDashTime = InvulnerableUntil = HitFlashUntil = MuzzleFlashUntil = 0;
    bFiring = false;
    bDashRequested = false;
    CastChecked<USphereComponent>(GetRootComponent())->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    MoveInput = FVector2D::ZeroVector;
    LastMoveDirection = FRotator(0, CameraAngle.Yaw, 0).Vector();
    UpdateCamera(0, true);
}

void AArenaPawn::ResetArena()
{
    if (AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
    {
        Mode->ResetArena();
    }
}
