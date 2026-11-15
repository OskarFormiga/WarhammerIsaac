#include "ArenaEnemy.h"
#include "ArenaCombatEffect.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "ArenaPawn.h"
#include "ArenaProjectile.h"
#include "ArenaVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AArenaEnemy::AArenaEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    UBoxComponent* Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
    Hitbox->SetBoxExtent(FVector(36, 36, 72));
    Hitbox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    SetRootComponent(Hitbox);
    Facing = CreateDefaultSubobject<USceneComponent>(TEXT("Facing"));
    Facing->SetupAttachment(Hitbox);
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Facing);
    Robot = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimatedRobot"));
    Robot->SetupAttachment(Facing);
    Robot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.44f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Alien(
        TEXT("/Game/PrototypeArt/Kenney/alien.alien"));
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Turret(
        TEXT("/Game/PrototypeArt/Kenney/turret_single.turret_single"));
    static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Rover(
        TEXT("/Game/PrototypeArt/Kenney/rover.rover"));
    TurretModel = Turret.Get();
    RoverModel = Rover.Get();
    if (UStaticMesh* Model = Alien.Get())
    {
        ArenaVisuals::FitCharacter(Body, Model, 155.0, 90.0);
        bUsesModel = true;
    }
}

void AArenaEnemy::SetType(EArenaEnemyType NewType)
{
    Type = NewType;
    const bool bDrone = Type == EArenaEnemyType::Turret;
    const bool bCrawler = Type == EArenaEnemyType::Rover;
    const TCHAR* Name = bDrone ? TEXT("Enemy_EyeDrone") : bCrawler ? TEXT("Enemy_Trilobite") : TEXT("Enemy_QuadShell");
    const FString Folder(TEXT("/Game/PrototypeArt/Upgrade/Robots/"));
    if (USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, *(Folder + Name + TEXT(".") + Name)))
    {
        const FBoxSphereBounds Bounds = Model->GetBounds();
        const double Height = bDrone ? 126.0 : bCrawler ? 116.0 : 146.0;
        const double Scale = Height / FMath::Max(1.0, Bounds.BoxExtent.Z * 2.0);
        const FRotator Rotation(0, -90, 0);
        Robot->SetSkeletalMeshAsset(Model);
        Robot->SetRelativeScale3D(FVector(Scale));
        Robot->SetRelativeRotation(Rotation);
        const double Hover = bDrone ? 55.0 : 0.0;
        Robot->SetRelativeLocation(FVector(0, 0, Height * 0.5 - 84.0 + Hover)
            - Rotation.RotateVector(Bounds.Origin * Scale));
        Body->SetVisibility(false);
        bUsesModel = true;
        const auto Animation = [&](const TCHAR* Clip)
        {
            const FString Asset = FString::Printf(TEXT("%s_Anim_Rig_%s"), Name, Clip);
            return LoadObject<UAnimSequence>(nullptr, *(Folder + Asset + TEXT(".") + Asset));
        };
        RobotIdle = Animation(TEXT("Idle"));
        RobotMove = bDrone ? RobotIdle.Get() : Animation(TEXT("Run"));
        RobotCharge = bDrone ? Animation(TEXT("Charging")) : bCrawler ? RobotIdle.Get() : Animation(TEXT("Charge"));
        RobotAttack = Animation(bCrawler ? TEXT("AttackAuto") : TEXT("Attack"));
        UpdateModelAnimation(GetWorld()->GetTimeSeconds(), false);
        return;
    }
    if (Type == EArenaEnemyType::Turret && TurretModel)
    {
        ArenaVisuals::FitCharacter(Body, TurretModel, 120.0, 90.0);
        bUsesModel = true;
    }
    else if (Type == EArenaEnemyType::Rover && RoverModel)
    {
        ArenaVisuals::FitCharacter(Body, RoverModel, 108.0, 90.0);
        bUsesModel = true;
    }
}

void AArenaEnemy::SetHealth(float Health)
{
    MaxHealth = RemainingHealth = FMath::Max(1.0f, Health);
    NextAttackTime = GetWorld()->GetTimeSeconds() + 1.5f;
    if (!bUsesModel)
    {
        ArenaVisuals::Tint(Body, FLinearColor(0.55f, 0.045f, 0.02f));
    }
}

void AArenaEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const float Now = GetWorld()->GetTimeSeconds();
    AArenaPawn* Player = Cast<AArenaPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player || Player->IsDead())
    {
        bWindingUp = false;
        return;
    }
    const bool bTurret = Type == EArenaEnemyType::Turret;
    const bool bRover = Type == EArenaEnemyType::Rover;
    const bool bRanged = bTurret || bRover;
    const FVector PreviousPosition = GetActorLocation();
    const FVector TowardPlayer = Player->GetActorLocation() - GetActorLocation();
    const double Distance = TowardPlayer.Size2D();
    const FVector Direction = TowardPlayer.GetSafeNormal2D();
    const float AttackRange = bTurret ? 1550.0f : bRover ? 930.0f : AttackRadius - 10;
    const FColor Warning = bTurret ? FColor(255, 65, 185) : FColor(255, 133, 35);

    if (bWindingUp)
    {
        if (bRanged)
        {
            const FVector Origin(GetActorLocation().X, GetActorLocation().Y, 5);
            DrawDebugLine(GetWorld(), Origin, Origin + AttackDirection * AttackRange, Warning, false, -1, 0, 3);
            if (bRover)
            {
                for (const float Angle : {-14.0f, 14.0f})
                {
                    const FVector Edge = AttackDirection.RotateAngleAxis(Angle, FVector::UpVector);
                    DrawDebugLine(GetWorld(), Origin, Origin + Edge * AttackRange, Warning, false, -1, 0, 2);
                }
            }
        }
        else
        {
            DrawDebugCircle(GetWorld(), AttackCenter, AttackRadius, 48, Warning,
                false, -1, 0, 3, FVector::ForwardVector, FVector::RightVector, false);
        }
        if (Now >= AttackAt)
        {
            RobotAttackUntil = Now + (RobotAttack ? FMath::Min(0.42f, RobotAttack->GetPlayLength()) : 0.0f);
            if (bRanged)
            {
                Shoot();
            }
            else if (FVector::DistSquared2D(Player->GetActorLocation(), AttackCenter) <= FMath::Square(AttackRadius + 30)
                && HasLineOfSight(*Player))
            {
                UGameplayStatics::ApplyDamage(Player, AttackDamage, nullptr, this, nullptr);
            }
            bWindingUp = false;
            NextAttackTime = Now + (bTurret ? 1.15f : bRover ? 1.05f : 0.65f);
        }
    }
    else if (Now >= NextAttackTime && Distance < AttackRange && HasLineOfSight(*Player))
    {
        bWindingUp = true;
        AttackAt = Now + (bTurret ? 0.9f : bRover ? 0.52f : WindupDuration);
        AttackCenter = FVector(GetActorLocation().X, GetActorLocation().Y, 8);
        AttackDirection = Direction;
    }
    else if (!bTurret)
    {
        FVector MoveDirection = Direction;
        if (bRover && Distance < 780.0 && HasLineOfSight(*Player))
        {
            const FVector Strafe(-Direction.Y, Direction.X, 0);
            MoveDirection = (Strafe + Direction * FMath::Clamp((Distance - 600.0) / 160.0, -1.0, 1.0))
                .GetSafeNormal2D();
        }
        MoveAroundCover(MoveDirection, DeltaTime);
    }
    Facing->SetWorldRotation((bWindingUp && bRanged ? AttackDirection : Direction).Rotation());
    UpdateModelAnimation(Now, FVector::DistSquared2D(PreviousPosition, GetActorLocation()) > 0.1);
    if (Now < HitFlashUntil || bWindingUp)
    {
        DrawDebugPoint(GetWorld(), GetActorLocation() + FVector(0, 0, bRanged ? 35 : 75),
            12, Now < HitFlashUntil ? FColor::White : Warning, false, -1);
    }
}

void AArenaEnemy::UpdateModelAnimation(float Now, bool bMoving)
{
    if (!Robot->GetSkeletalMeshAsset())
    {
        return;
    }
    UAnimSequence* Pose = Now < RobotAttackUntil ? RobotAttack.Get()
        : bWindingUp ? RobotCharge.Get() : bMoving ? RobotMove.Get() : RobotIdle.Get();
    const UAnimSingleNodeInstance* Instance = Robot->GetSingleNodeInstance();
    if (Pose && (!Instance || Robot->GetSingleNodeInstance()->GetCurrentAsset() != Pose))
    {
        Robot->PlayAnimation(Pose, Pose != RobotAttack);
    }
}

void AArenaEnemy::MoveAroundCover(FVector Direction, float DeltaTime)
{
    const float Speed = MoveSpeed * (Type == EArenaEnemyType::Rover ? 1.45f : 1.28f);
    FHitResult Hit;
    AddActorWorldOffset(Direction * Speed * DeltaTime, true, &Hit);
    if (Hit.bBlockingHit)
    {
        FVector Slide = FVector::VectorPlaneProject(Direction, Hit.Normal).GetSafeNormal2D();
        if (Slide.IsNearlyZero())
        {
            Slide = FVector(-Direction.Y, Direction.X, 0);
        }
        AddActorWorldOffset(Slide * Speed * DeltaTime * (1.0f - Hit.Time), true);
    }
}

bool AArenaEnemy::HasLineOfSight(const AArenaPawn& Player) const
{
    FHitResult Obstacle;
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(this);
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Obstacle,
        GetActorLocation(), Player.GetActorLocation(), ECC_Visibility, Query);
    return !bBlocked || Obstacle.GetActor() == &Player;
}

void AArenaEnemy::Shoot()
{
    const bool bRover = Type == EArenaEnemyType::Rover;
    FActorSpawnParameters Spawn;
    Spawn.Owner = this;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const int32 Spread = bRover ? 1 : 0;
    for (int32 Index = -Spread; Index <= Spread; ++Index)
    {
        const FVector Direction = AttackDirection.RotateAngleAxis(Index * 14.0f, FVector::UpVector);
        if (AArenaProjectile* Shot = GetWorld()->SpawnActor<AArenaProjectile>(
            GetActorLocation(), Direction.Rotation(), Spawn))
        {
            Shot->Launch(bRover ? 740.0f : 1270.0f, bRover ? 10.0f : 22.0f,
                bRover ? 1.5f : 1.65f, true, bRover);
        }
    }
}

float AArenaEnemy::TakeDamage(float Amount, const FDamageEvent& Event,
    AController* InstigatorController, AActor* Causer)
{
    const float Applied = FMath::Clamp(Amount, 0.0f, RemainingHealth);
    RemainingHealth -= Applied;
    if (RemainingHealth <= 0)
    {
        AArenaCombatEffect::Emit(*GetWorld(), GetActorLocation(), FVector::UpVector,
            FLinearColor(1.0f, 0.34f, 0.08f), EArenaEffect::Impact);
        Destroy();
    }
    else if (Applied > 0)
    {
        HitFlashUntil = GetWorld()->GetTimeSeconds() + 0.12f;
    }
    return Applied;
}
