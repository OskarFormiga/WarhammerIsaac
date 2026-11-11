#include "ArenaEnemy.h"
#include "ArenaPawn.h"
#include "ArenaProjectile.h"
#include "ArenaVisuals.h"
#include "Components/BoxComponent.h"
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
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Hitbox);
    Body->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.44f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(Cube.Object);
}

void AArenaEnemy::SetHealth(float Health)
{
    MaxHealth = RemainingHealth = FMath::Max(1.0f, Health);
    NextAttackTime = GetWorld()->GetTimeSeconds() + 1.5f;
    ArenaVisuals::Tint(Body, FLinearColor(0.55f, 0.045f, 0.02f));
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
    const FVector TowardPlayer = Player->GetActorLocation() - GetActorLocation();
    if (bWindingUp)
    {
        if (!bRanged)
        {
            DrawDebugCircle(GetWorld(), AttackCenter, AttackRadius, 48, FColor(255, 85, 25),
                false, -1, 0, 3, FVector::ForwardVector, FVector::RightVector, false);
        }
        if (Now >= AttackAt)
        {
            if (bRanged)
            {
                Shoot(*Player);
            }
            else
            {
                if (FVector::DistSquared2D(Player->GetActorLocation(), AttackCenter) <= FMath::Square(AttackRadius + 30)
                    && HasLineOfSight(*Player))
                {
                    UGameplayStatics::ApplyDamage(Player, AttackDamage, nullptr, this, nullptr);
                }
                DrawDebugCircle(GetWorld(), AttackCenter, AttackRadius, 48, FColor::Orange,
                    false, 0.12f, 0, 4, FVector::ForwardVector, FVector::RightVector, false);
            }
            bWindingUp = false;
            NextAttackTime = Now + (bRanged ? 1.45f : 0.8f);
        }
    }
    else if (Now >= NextAttackTime && TowardPlayer.SizeSquared2D() < FMath::Square(bRanged ? 1050.0f : AttackRadius - 10)
        && HasLineOfSight(*Player))
    {
        bWindingUp = true;
        AttackAt = Now + WindupDuration;
        AttackCenter = FVector(GetActorLocation().X, GetActorLocation().Y, 8);
    }
    else if (!bRanged || TowardPlayer.SizeSquared2D() > FMath::Square(850.0f) || !HasLineOfSight(*Player))
    {
        const FVector Direction = TowardPlayer.GetSafeNormal2D();
        FHitResult Hit;
        AddActorWorldOffset(Direction * MoveSpeed * DeltaTime, true, &Hit);
        if (Hit.bBlockingHit)
        {
            FVector Slide = FVector::VectorPlaneProject(Direction, Hit.Normal).GetSafeNormal2D();
            if (Slide.IsNearlyZero())
            {
                Slide = FVector(-Direction.Y, Direction.X, 0);
            }
            AddActorWorldOffset(Slide * MoveSpeed * DeltaTime, true);
        }
        Body->SetWorldRotation(Direction.Rotation());
    }
    FLinearColor Color = bRanged ? FLinearColor(0.42f, 0.06f, 0.62f) : FLinearColor(0.55f, 0.045f, 0.02f);
    if (Now < HitFlashUntil)
    {
        Color = FLinearColor::White;
    }
    else if (bWindingUp)
    {
        Color = bRanged ? FLinearColor(1.0f, 0.35f, 0.95f) : FLinearColor(1, 0.32f, 0.025f);
    }
    ArenaVisuals::Tint(Body, Color);
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

void AArenaEnemy::Shoot(const AArenaPawn& Player)
{
    const FVector Direction = (Player.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
    FActorSpawnParameters Spawn;
    Spawn.Owner = this;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AArenaProjectile* Shot = GetWorld()->SpawnActor<AArenaProjectile>(
        GetActorLocation(), Direction.Rotation(), Spawn))
    {
        Shot->Launch(650.0f, 15.0f, 2.5f, true);
    }
}

float AArenaEnemy::TakeDamage(float Amount, const FDamageEvent& Event,
    AController* InstigatorController, AActor* Causer)
{
    const float Applied = FMath::Clamp(Amount, 0.0f, RemainingHealth);
    RemainingHealth -= Applied;
    if (RemainingHealth <= 0)
    {
        DrawDebugPoint(GetWorld(), GetActorLocation(), 16, FColor(255, 200, 110), false, 0.12f);
        Destroy();
    }
    else if (Applied > 0)
    {
        HitFlashUntil = GetWorld()->GetTimeSeconds() + 0.12f;
    }
    return Applied;
}
