#include "ArenaProjectile.h"

#include "ArenaVisuals.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AArenaProjectile::AArenaProjectile()
{
    USphereComponent* Hitbox = CreateDefaultSubobject<USphereComponent>(TEXT("Hitbox"));
    Hitbox->InitSphereRadius(9.0f);
    Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Hitbox->SetCollisionObjectType(ECC_GameTraceChannel1);
    Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    Hitbox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Hitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    SetRootComponent(Hitbox);

    UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Mesh->SetupAttachment(Hitbox);
    Mesh->SetRelativeScale3D(FVector(0.18));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Mesh->SetStaticMesh(Sphere.Object);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Hitbox);
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bShouldBounce = false;
    Movement->OnProjectileStop.AddDynamic(this, &AArenaProjectile::OnImpact);
}

void AArenaProjectile::Launch(float Speed, float Damage, float Lifetime, bool bHostile)
{
    if (bHostile)
    {
        USphereComponent* Hitbox = CastChecked<USphereComponent>(GetRootComponent());
        Hitbox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
        Hitbox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        Hitbox->SetSphereRadius(19.0f);
    }
    ImpactDamage = Damage;
    Movement->Velocity = GetActorForwardVector() * Speed;
    SetLifeSpan(Lifetime);
    UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
    if (bHostile)
    {
        Mesh->SetRelativeScale3D(FVector(0.38));
    }
    ArenaVisuals::Tint(Mesh, bHostile ? FLinearColor(1.0f, 0.08f, 0.55f)
        : FLinearColor(1.0f, 0.75f, 0.06f));
}

void AArenaProjectile::OnImpact(const FHitResult& Hit)
{
    DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 7, FColor(255, 220, 140), false, 0.08f);
    UGameplayStatics::ApplyDamage(Hit.GetActor(), ImpactDamage, GetInstigatorController(),
        this, nullptr);
    Destroy();
}
