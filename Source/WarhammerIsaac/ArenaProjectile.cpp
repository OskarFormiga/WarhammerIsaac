#include "ArenaProjectile.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ArenaCombatEffect.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AArenaProjectile::AArenaProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    USphereComponent* Hitbox = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
    Hitbox->InitSphereRadius(9.0f);
    Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Hitbox->SetCollisionObjectType(ECC_GameTraceChannel1);
    Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    Hitbox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Hitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    SetRootComponent(Hitbox);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Emissive(
        TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    EmissiveMaterial = Emissive.Object;

    Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
    Core->SetupAttachment(Hitbox);
    Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Core->SetCastShadow(false);
    Core->SetStaticMesh(Sphere.Object);

    Tracer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tracer"));
    Tracer->SetupAttachment(Hitbox);
    Tracer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Tracer->SetCastShadow(false);
    Tracer->SetStaticMesh(Sphere.Object);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Hitbox);
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bShouldBounce = false;
    Movement->OnProjectileStop.AddDynamic(this, &AArenaProjectile::OnImpact);
}

void AArenaProjectile::Launch(float Speed, float Damage, float Lifetime, bool bHostile, bool bSpreadShot)
{
    if (bHostile)
    {
        USphereComponent* Hitbox = CastChecked<USphereComponent>(GetRootComponent());
        Hitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
        Hitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        Hitbox->SetSphereRadius(bSpreadShot ? 15.0f : 21.0f);
    }
    ImpactDamage = Damage;
    Movement->Velocity = GetActorForwardVector() * Speed;
    SetLifeSpan(Lifetime);
    LaunchPoint = GetActorLocation();
    ShotColor = !bHostile ? FLinearColor(1.0f, 0.83f, 0.46f)
        : bSpreadShot ? FLinearColor(1.0f, 0.27f, 0.05f) : FLinearColor(1.0f, 0.06f, 0.43f);
    TracerWidth = !bHostile ? 0.040f : bSpreadShot ? 0.12f : 0.19f;
    TracerLength = !bHostile ? 195.0f : bSpreadShot ? 92.0f : 150.0f;
    Core->SetRelativeScale3D(FVector(TracerWidth * 2.0f, TracerWidth, TracerWidth));
    Tick(0.0f);
    if (UMaterialInstanceDynamic* Material = Core->CreateDynamicMaterialInstance(0, EmissiveMaterial))
    {
        Material->SetVectorParameterValue(TEXT("Color"), ShotColor * 7.0f);
    }
    if (UMaterialInstanceDynamic* Material = Tracer->CreateDynamicMaterialInstance(0, EmissiveMaterial))
    {
        Material->SetVectorParameterValue(TEXT("Color"), ShotColor * 3.5f);
    }
}

void AArenaProjectile::SetPlayerTracer(const FVector& Muzzle, float Visibility)
{
    bShowImpact = Visibility > 0.0f;
    Core->SetVisibility(Visibility >= 1.0f);
    Tracer->SetVisibility(Visibility > 0.0f);
    if (Visibility > 0.0f)
    {
        VisualOffset = (Muzzle - LaunchPoint).GetClampedToMaxSize(85.0);
        TracerWidth *= Visibility;
        TracerLength *= Visibility;
        Tick(0.0f);
    }
    else
    {
        SetActorTickEnabled(false);
    }
}

void AArenaProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    const double Travel = FVector::Dist(GetActorLocation(), LaunchPoint);
    const double Length = FMath::Clamp(Travel + 4.0, 4.0, static_cast<double>(TracerLength));
    const FVector Offset = VisualOffset * (1.0 - FMath::SmoothStep(0.0, 220.0, Travel));
    Core->SetWorldLocation(GetActorLocation() + Offset);
    Tracer->SetWorldLocation(GetActorLocation() + Offset - GetActorForwardVector() * (Length * 0.5));
    Tracer->SetRelativeScale3D(FVector(Length / 100.0, TracerWidth, TracerWidth));
}

void AArenaProjectile::OnImpact(const FHitResult& Hit)
{
    if (bShowImpact)
    {
        const FVector Normal = Hit.ImpactNormal.IsNearlyZero() ? -GetActorForwardVector() : FVector(Hit.ImpactNormal);
        AArenaCombatEffect::Emit(*GetWorld(), Hit.ImpactPoint + Normal * 5.0, Normal, ShotColor, EArenaEffect::Impact);
    }
    UGameplayStatics::ApplyDamage(Hit.GetActor(), ImpactDamage, GetInstigatorController(), this, nullptr);
    Destroy();
}
