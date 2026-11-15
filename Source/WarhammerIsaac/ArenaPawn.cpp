#include "ArenaPawn.h"
#include "ArenaCombatEffect.h"
#include "ArenaGameMode.h"
#include "ArenaProjectile.h"
#include "ArenaVisuals.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    const FRotator SoldierFacing(0, -90, 0);

    UAnimSequence* LoadQuaterniusAnimation(const TCHAR* Folder, const TCHAR* Name)
    {
        const FString Path = FString::Printf(TEXT("/Game/PrototypeArt/Quaternius/%s/%s.%s"), Folder, Name, Name);
        return LoadObject<UAnimSequence>(nullptr, *Path);
    }

    const FVector BarrelRestPosition(60, 18, 6);
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
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Astronaut(
        TEXT("/Game/PrototypeArt/Kenney/astronautA.astronautA"));
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Rifle(
        TEXT("/Game/PrototypeArt/Kenney/weapon_rifle.weapon_rifle"));
    AimPivot = CreateDefaultSubobject<USceneComponent>(TEXT("AimPivot"));
    AimPivot->SetupAttachment(Hitbox);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(AimPivot);
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
    if (UStaticMesh* Model = Astronaut.Get())
    {
        ArenaVisuals::FitCharacter(Body, Model, 172.0, 90.0);
        Head->SetVisibility(false);
        Legs->SetVisibility(false);
        bUsesCharacterModel = true;
    }

    WeaponMount = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponMount"));
    WeaponMount->SetupAttachment(AimPivot);
    WeaponMount->SetRelativeLocation(BarrelRestPosition);
    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
    Barrel->SetupAttachment(WeaponMount);
    Barrel->SetStaticMesh(Cube.Object);
    Barrel->SetRelativeScale3D(FVector(0.68, 0.28, 0.28));
    Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (UStaticMesh* Model = Rifle.Get())
    {
        const FVector Extent = Model->GetBounds().BoxExtent;
        const double Scale = 76.0 / FMath::Max(1.0, FMath::Max(Extent.X, Extent.Y) * 2.0);
        ArenaVisuals::PlaceModel(Barrel, Model, Scale, FRotator(0, Extent.Y > Extent.X ? 90 : 0, 0),
            FVector::ZeroVector);
        bUsesWeaponModel = true;
    }

    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
    MuzzleLight->SetupAttachment(WeaponMount);
    MuzzleLight->SetRelativeLocation(FVector(40, 0, 0));
    MuzzleLight->SetLightColor(FLinearColor(1, 0.62f, 0.18f));
    MuzzleLight->SetIntensity(0);
    MuzzleLight->SetAttenuationRadius(320);
    MuzzleLight->SetCastShadows(false);

    static ConstructorHelpers::FObjectFinderOptional<USkeletalMesh> Soldier(
        TEXT("/Game/PrototypeArt/Quaternius/Soldier/Soldier.Soldier"));
    AnimatedBody = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimatedSoldier"));
    AnimatedBody->SetupAttachment(AimPivot);
    AnimatedBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (USkeletalMesh* Mesh = Soldier.Get())
    {
        AnimatedBody->SetSkeletalMeshAsset(Mesh);
        AnimatedBody->SetRelativeScale3D(FVector(172.0 / FMath::Max(1.0, Mesh->GetBounds().BoxExtent.Z * 2.0)));
        static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SoldierSkin(
            TEXT("/Game/PrototypeArt/Quaternius/Soldier/Skin.Skin"));
        if (UMaterialInterface* Skin = SoldierSkin.Get())
        {
            const int32 SkinSlot = AnimatedBody->GetMaterialIndex(TEXT("Skin"));
            if (SkinSlot != INDEX_NONE)
            {
                AnimatedBody->SetMaterial(SkinSlot, Skin);
            }
        }
        IdleAnimation = LoadQuaterniusAnimation(TEXT("Soldier"), TEXT("Soldier_Anim_CharacterArmature_Idle"));
        RunAnimation = LoadQuaterniusAnimation(TEXT("Soldier"), TEXT("Soldier_Anim_CharacterArmature_Run_Gun"));
        ShootAnimation = LoadQuaterniusAnimation(TEXT("Soldier"), TEXT("Soldier_Anim_CharacterArmature_Idle_Shoot"));
        DuckAnimation = LoadQuaterniusAnimation(TEXT("Soldier"), TEXT("Soldier_Anim_CharacterArmature_Duck"));
        DeathAnimation = LoadQuaterniusAnimation(TEXT("Soldier"), TEXT("Soldier_Anim_CharacterArmature_Death"));
        AnimatedBody->OverrideAnimationData(ShootAnimation ? ShootAnimation : IdleAnimation, false, false);
        Body->SetVisibility(false, true);
        bUsesCharacterModel = true;
    }

    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Shotgun(
        TEXT("/Game/PrototypeArt/Upgrade/Workshop/BreachShotgun.BreachShotgun"));
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Pump(
        TEXT("/Game/PrototypeArt/Upgrade/Workshop/BreachPump.BreachPump"));
    ShotgunPump = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShotgunPump"));
    ShotgunPump->SetupAttachment(WeaponMount);
    ShotgunPump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (UStaticMesh* Mesh = Shotgun.Get())
    {
        Barrel->SetStaticMesh(Mesh);
        Barrel->SetRelativeTransform(FTransform(FQuat::Identity, FVector::ZeroVector, FVector(1.0, 1.18, 1.12)));
        ShotgunPump->SetStaticMesh(Pump.Get());
        ShotgunPump->SetRelativeScale3D(Barrel->GetRelativeScale3D());
        bUsesWeaponModel = true;
    }

    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Armor(
        TEXT("/Game/PrototypeArt/Upgrade/Workshop/MarineArmor.MarineArmor"));
    MarineArmor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarineArmor"));
    MarineArmor->SetupAttachment(AimPivot);
    MarineArmor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarineArmor->SetStaticMesh(Armor.Get());
    MarineArmor->SetRelativeScale3D(FVector(1.13));

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
    Camera->SetupAttachment(Hitbox);
    Camera->SetAbsolute(true, true, true);
    Camera->bConstrainAspectRatio = true;
    Camera->AspectRatio = 16.0f / 9.0f;
    UFloatingPawnMovement* Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Hitbox);
    Movement->MaxSpeed = RunSpeed;
    Movement->Acceleration = 8800;
    Movement->Deceleration = 10500;
    Movement->TurningBoost = 22;
    Movement->SetPlaneConstraintEnabled(true);
    Movement->SetPlaneConstraintNormal(FVector::UpVector);
}

void AArenaPawn::BeginPlay()
{
    Super::BeginPlay();
    ResetForRound();
    UpdateCharacterAnimation(GetWorld()->GetTimeSeconds(), 0.0f);
    if (!bUsesWeaponModel)
    {
        ArenaVisuals::Tint(Barrel, FLinearColor(0.35f, 0.29f, 0.19f));
    }
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
    Input->BindAction(TEXT("Slide"), IE_Pressed, this, &AArenaPawn::RequestSlide);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &AArenaPawn::StartSprint);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &AArenaPawn::StopSprint);
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
    UFloatingPawnMovement* Movement = CastChecked<UFloatingPawnMovement>(GetMovementComponent());
    Movement->MaxSpeed = bSprinting ? SprintSpeed : RunSpeed;
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
        if (SlideBufferedUntil > Now)
        {
            TryStartSlide(Now);
        }
        if (IsDashing())
        {
            AdvanceDash(DeltaTime);
        }
        if (IsSliding())
        {
            AdvanceSlide(DeltaTime);
        }
        if (!IsDashing() && !IsSliding())
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
    WeaponMount->SetRelativeLocation(BarrelRestPosition - FVector(22.0f * Recoil * Recoil, 0, 28.0f * ReloadPose));
    WeaponMount->SetRelativeRotation(FRotator(-35.0f * ReloadPose, 0, 0));
    UpdateCharacterAnimation(Now, DeltaTime);
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
    if (!bUsesCharacterModel)
    {
        ArenaVisuals::Tint(Body, Color);
    }
    else if (!IsDead())
    {
        const FColor Marker = Now < HitFlashUntil ? FColor::Red : FColor::Cyan;
        DrawDebugCircle(GetWorld(), GetActorLocation() - FVector(0, 0, 81), 47, 32, Marker,
            false, -1, 0, Now < DashInvulnerableUntil ? 5.0f : 2.0f,
            FVector::ForwardVector, FVector::RightVector, false);
    }
    if (bShowCameraBounds)
    {
        DrawDebugBox(GetWorld(), AArenaGameMode::CameraRoomCenter(GetActorLocation()) + FVector(0, 0, 10),
            FVector(CameraFollowExtent.X, CameraFollowExtent.Y, 2),
            bCameraFollowing ? FColor::Cyan : FColor::Orange, false, -1, 0, 3);
    }
}

void AArenaPawn::UpdateCharacterAnimation(float Now, float DeltaTime)
{
    const bool bRolling = IsDashing();
    const bool bSliding = IsSliding();
    const FVector Stride = bSliding ? SlideDirection * 850.0 : GetVelocity();
    VisualStride = FMath::Lerp(VisualStride, Stride, 1.0 - FMath::Exp(-17.0 * DeltaTime));
    const float ShotAge = Now - (MuzzleFlashUntil - FlashDuration);
    const bool bRecentShot = MuzzleFlashUntil > 0.0f && ShotAge < RecoilDuration;
    if (const USkeletalMesh* Mesh = AnimatedBody->GetSkeletalMeshAsset())
    {
        const float Roll = bRolling ? 1.0f - DashTimeRemaining / FMath::Max(0.01f, DashDuration) : 0.0f;
        const float RollYaw = bRolling ? DashDirection.Rotation().Yaw - AimPivot->GetComponentRotation().Yaw : 0.0f;
        const float SlideProgress = bSliding ? 1.0f - SlideTimeRemaining / FMath::Max(0.1f, SlideDuration) : 0.0f;
        const float Crouch = bSliding ? FMath::SmoothStep(0.0f, 0.18f, SlideProgress)
            * (1.0f - FMath::SmoothStep(0.70f, 1.0f, SlideProgress)) : 0.0f;
        const float ShotKick = !bRolling && bRecentShot && !IsDead()
            ? 6.0f * FMath::Square(1.0f - ShotAge / RecoilDuration) : 0.0f;
        const FVector LocalStride = AimPivot->GetComponentTransform().InverseTransformVectorNoScale(VisualStride) / 750.0;
        const float PoseYaw = bRolling ? RollYaw : 24.0f * FMath::Clamp(LocalStride.Y, -1.0, 1.0);
        const float Lean = bRolling || IsDead() ? 0.0f : -10.0f * FMath::Clamp(LocalStride.X, -1.0, 1.0);
        const float Bank = bRolling || IsDead() ? 0.0f : 13.0f * FMath::Clamp(LocalStride.Y, -1.0, 1.0);
        const FQuat Facing = FRotator(0, PoseYaw, 0).Quaternion()
            * FRotator(-360.0f * FMath::SmoothStep(0.0f, 1.0f, Roll) + ShotKick + Lean, 0, Bank).Quaternion()
            * SoldierFacing.Quaternion();
        const double Speed = VisualStride.Size2D();
        const double Bounce = !bSliding && !bRolling && !IsDead()
            ? FMath::Sin(Now * 29.0) * FMath::Clamp(Speed / 280.0, 0.0, 2.7) : 0.0;
        const FVector Center(0, 0, 5.0 - 20.0 * FMath::Sin(PI * Roll) - 22.0 * Crouch + Bounce);
        AnimatedBody->SetRelativeRotation(Facing);
        AnimatedBody->SetRelativeLocation(Center - Facing.RotateVector(
            Mesh->GetBounds().Origin * AnimatedBody->GetRelativeScale3D()));

        const bool bMoving = Speed > 55.0;
        UAnimSequence* Pose = IdleAnimation;
        if (IsDead())
        {
            Pose = DeathAnimation;
        }
        else if (bRolling || bSliding)
        {
            Pose = DuckAnimation;
        }
        else if (bMoving)
        {
            Pose = RunAnimation;
        }
        else if (ShootAnimation)
        {
            Pose = ShootAnimation;
        }
        if (Pose && (!AnimatedBody->GetSingleNodeInstance() || AnimatedBody->GetSingleNodeInstance()->GetCurrentAsset() != Pose))
        {
            AnimatedBody->PlayAnimation(Pose, Pose == IdleAnimation || Pose == RunAnimation);
            if (Pose == DuckAnimation || Pose == ShootAnimation)
            {
                AnimatedBody->Stop();
            }
        }
        if (Pose && Pose == DuckAnimation)
        {
            const float Sample = bRolling ? 0.5f : FMath::Lerp(0.08f, 0.52f, Crouch);
            AnimatedBody->SetPosition(Pose->GetPlayLength() * Sample, false);
        }
        else if (Pose && Pose == RunAnimation)
        {
            const double Direction = LocalStride.X < -0.22 ? -1.0 : 1.0;
            AnimatedBody->SetPlayRate(Direction * FMath::Clamp(Speed / 520.0, 0.45, 1.48));
        }
        else if (Pose && Pose == ShootAnimation)
        {
            const float Progress = bRecentShot ? FMath::Clamp(ShotAge / RecoilDuration, 0.0f, 1.0f) : 0.0f;
            AnimatedBody->SetPosition(Pose->GetPlayLength() * Progress, false);
        }
    }
    Barrel->SetVisibility(!bRolling && !IsDead());
    ShotgunPump->SetVisibility(!bRolling && !IsDead());
    static const FName GripBone(TEXT("Middle1.R"));
    static const FName SanitizedGripBone(TEXT("Middle1_R"));
    const FName RightGrip = AnimatedBody->GetBoneIndex(GripBone) != INDEX_NONE ? GripBone : SanitizedGripBone;
    if (AnimatedBody->GetBoneIndex(RightGrip) != INDEX_NONE && !bRolling && !IsDead())
    {
        const FVector Hand = AimPivot->GetComponentTransform().InverseTransformPosition(
            AnimatedBody->GetSocketLocation(RightGrip));
        const float Kick = bRecentShot ? 11.0f * (1.0f - ShotAge / RecoilDuration) : 0.0f;
        WeaponMount->SetRelativeLocation(Hand + FVector(11.0f - Kick, 0, -7.0f * ReloadPose));
    }
    const float PumpPhase = IsReloading() ? FMath::Clamp((GetReloadProgress() - 0.75f) / 0.25f, 0.0f, 1.0f)
        : MuzzleFlashUntil > 0.0f ? FMath::Clamp((ShotAge - 0.055f) / 0.28f, 0.0f, 1.0f) : 0.0f;
    const float PumpTravel = FMath::SmoothStep(0.0f, 0.43f, PumpPhase)
        * (1.0f - FMath::SmoothStep(0.49f, 1.0f, PumpPhase));
    ShotgunPump->SetRelativeLocation(FVector(-14.0f * PumpTravel, 0, 0));

    static const FName Torso(TEXT("Torso"));
    if (MarineArmor->GetStaticMesh() && AnimatedBody->GetBoneIndex(Torso) != INDEX_NONE)
    {
        const FQuat BodyFacing = AnimatedBody->GetComponentQuat() * SoldierFacing.Quaternion().Inverse();
        MarineArmor->SetWorldLocationAndRotation(
            AnimatedBody->GetSocketLocation(Torso) + BodyFacing.RotateVector(FVector(0, 0, 3)), BodyFacing);
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
    const FVector Lead = !bSnap && !bInPassage && bCameraFollowing && !IsDead()
        ? IsSliding() ? SlideDirection * 88.0
            : FVector(GetVelocity().X, GetVelocity().Y, 0).GetClampedToMaxSize(750.0) * 0.075
        : FVector::ZeroVector;
    CameraMovementLead = bSnap ? FVector::ZeroVector
        : FMath::Lerp(CameraMovementLead, Lead, 1.0 - FMath::Exp(-11.0 * DeltaTime));
    if (!bInPassage)
    {
        FollowTarget += CameraMovementLead;
    }
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
        const double Alpha = 1.0 - FMath::Exp(-10.0 * FollowWeight * DeltaTime);
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
        AimPivot->SetWorldRotation(Aim.Rotation());
    }
}

FVector AArenaPawn::GetVisualMuzzle() const
{
    const FVector Position = WeaponMount->GetComponentTransform().TransformPosition(FVector(64.6, 0, 8.2));
    if (FVector::DotProduct(Position - GetActorLocation(), AimPivot->GetForwardVector()) > 25.0)
    {
        return Position;
    }
    return GetActorLocation() + AimPivot->GetForwardVector() * 76.0 + FVector(0, 0, 22);
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
    const FRotator Aim = AimPivot->GetComponentRotation();
    const FVector Origin = GetActorLocation();
    const FVector Muzzle = Origin + Aim.Vector() * 67.0f;
    FCollisionQueryParams MuzzleQuery(SCENE_QUERY_STAT(ArenaMuzzle), false, this);
    const bool bMuzzleBlocked = GetWorld()->SweepTestByObjectType(Origin, Muzzle, FQuat::Identity,
        FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic)),
        FCollisionShape::MakeSphere(9.0f), MuzzleQuery);
    const FVector ShotOrigin = bMuzzleBlocked ? Origin : Muzzle;
    const FVector VisualMuzzle = bMuzzleBlocked ? Origin : GetVisualMuzzle();
    AArenaCombatEffect::Emit(*GetWorld(), VisualMuzzle, Aim.Vector(), FLinearColor(1.0f, 0.60f, 0.22f),
        EArenaEffect::Muzzle);
    const int32 Count = FMath::Clamp(PelletCount, 1, 16);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Offset = Count > 1 ? FMath::Lerp(-SpreadHalfAngle, SpreadHalfAngle,
            static_cast<float>(Index) / (Count - 1)) : 0.0f;
        const FRotator PelletDirection(0, Aim.Yaw + Offset, 0);
        if (AArenaProjectile* Pellet = GetWorld()->SpawnActor<AArenaProjectile>(
            ShotOrigin, PelletDirection, Spawn))
        {
            Pellet->Launch(ProjectileSpeed, PelletDamage * DamageMultiplier, ProjectileLifetime);
            const int32 FromCenter = FMath::Abs(Index - Count / 2);
            Pellet->SetPlayerTracer(VisualMuzzle, FromCenter == 0 ? 1.0f : FromCenter == 1 ? 0.47f : 0.0f);
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

void AArenaPawn::RequestSlide()
{
    SlideBufferedUntil = GetWorld()->GetTimeSeconds() + 0.15f;
}

void AArenaPawn::TryStartSlide(float Now)
{
    if (IsDashing() || IsSliding() || Now < NextSlideTime)
    {
        return;
    }
    const FVector Direction = GetMoveDirection().GetSafeNormal2D();
    if (Direction.IsNearlyZero())
    {
        return;
    }
    SlideDirection = Direction;
    SlideTimeRemaining = FMath::Max(0.1f, SlideDuration);
    NextSlideSpark = Now;
    NextSlideTime = Now + SlideTimeRemaining + 0.20f;
    SlideBufferedUntil = 0.0f;
    GetMovementComponent()->StopMovementImmediately();
    ConsumeMovementInputVector();
}

void AArenaPawn::AdvanceSlide(float DeltaTime)
{
    const float Duration = FMath::Max(0.1f, SlideDuration);
    const float Step = FMath::Min(SlideTimeRemaining, DeltaTime);
    const float Progress = 1.0f - (SlideTimeRemaining - Step * 0.5f) / Duration;
    const FVector InputDirection = GetMoveDirection().GetSafeNormal2D();
    if (!InputDirection.IsNearlyZero())
    {
        const float Yaw = FMath::FixedTurn(SlideDirection.Rotation().Yaw, InputDirection.Rotation().Yaw, 96.0f * Step);
        SlideDirection = FRotator(0, Yaw, 0).Vector();
    }

    UFloatingPawnMovement* Movement = CastChecked<UFloatingPawnMovement>(GetMovementComponent());
    const float ExitSpeed = Movement->MaxSpeed * 0.84f;
    const float Speed = FMath::Lerp(FMath::Max(SlideStartSpeed, ExitSpeed), ExitSpeed, Progress);
    const FVector Delta = SlideDirection * Speed * Step;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now >= NextSlideSpark)
    {
        NextSlideSpark = Now + 0.052f;
        const FVector Contact = GetActorLocation() - SlideDirection * 37.0 + FVector(0, 0, -77);
        AArenaCombatEffect::Emit(*GetWorld(), Contact, -SlideDirection, FLinearColor(0.82f, 0.42f, 0.13f),
            EArenaEffect::Slide);
    }
    FHitResult Hit;
    AddActorWorldOffset(Delta, true, &Hit);
    SlideTimeRemaining = FMath::Max(0.0f, SlideTimeRemaining - Step);
    if (Hit.bBlockingHit)
    {
        const FVector Tangent = FVector::VectorPlaneProject(SlideDirection, Hit.Normal);
        if (Tangent.SizeSquared2D() < 0.36 || Hit.bStartPenetrating)
        {
            SlideTimeRemaining = 0.0f;
            return;
        }
        SlideDirection = Tangent.GetSafeNormal2D();
        FHitResult Corner;
        AddActorWorldOffset(SlideDirection * Speed * Step * (1.0f - Hit.Time), true, &Corner);
        if (Corner.bBlockingHit)
        {
            SlideTimeRemaining = 0.0f;
            return;
        }
    }
    if (!IsSliding() && !InputDirection.IsNearlyZero())
    {
        Movement->Velocity = SlideDirection * ExitSpeed;
    }
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
    SlideTimeRemaining = 0.0f;
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
    // Keep a quick initial response, with a softer finish as the soldier lands.
    const float DistanceFraction = (End - Start) * (1.0f + 0.22f * (1.0f - Start - End));
    FHitResult Hit;
    AddActorWorldOffset(DashDirection * (DashSpeed * Duration * DistanceFraction), true, &Hit);
    if (Hit.bBlockingHit || DashTimeRemaining <= 0.0f)
    {
        CombatAction = EArenaCombatAction::Idle;
        DashTimeRemaining = 0.0f;
        const FVector Direction = GetMoveDirection();
        if (!Hit.bBlockingHit && !Direction.IsNearlyZero())
        {
            UFloatingPawnMovement* Movement = CastChecked<UFloatingPawnMovement>(GetMovementComponent());
            Movement->Velocity = Direction * Movement->MaxSpeed * 0.68f;
        }
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
        SlideTimeRemaining = SlideBufferedUntil = 0.0f;
        bSprinting = false;
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
    SlideTimeRemaining = SlideBufferedUntil = NextSlideTime = NextSlideSpark = 0.0f;
    VisualStride = FVector::ZeroVector;
    bSprinting = false;
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
