#include "ArenaCombatEffect.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AArenaCombatEffect::AArenaCombatEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Emissive(
        TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("Particle%d"), Index));
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->SetStaticMesh(Sphere.Object);
        Mesh->SetMaterial(0, Emissive.Object);
        Mesh->SetVisibility(false);
        Meshes.Add(Mesh);
    }
}

void AArenaCombatEffect::Emit(UWorld& World, const FVector& Origin, const FVector& Direction,
    const FLinearColor& Color, EArenaEffect Effect)
{
    if (World.GetNetMode() == NM_DedicatedServer || !FApp::CanEverRender())
    {
        return;
    }
    AArenaCombatEffect* Burst = World.SpawnActor<AArenaCombatEffect>(Origin, FRotator::ZeroRotator);
    if (!Burst)
    {
        return;
    }
    const bool bMuzzle = Effect == EArenaEffect::Muzzle;
    const bool bSlide = Effect == EArenaEffect::Slide;
    Burst->Tint = Color;
    Burst->Duration = bMuzzle ? 0.065f : bSlide ? 0.23f : 0.17f;
    Burst->Gravity = bMuzzle ? 0.0f : 420.0f;
    Burst->Glow = Burst->Meshes[0]->CreateDynamicMaterialInstance(0);
    const int32 Count = bMuzzle ? 5 : bSlide ? 3 : 6;
    const FVector Axis = Direction.GetSafeNormal();
    const FVector Side = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FVector Scatter = Side * FMath::FRandRange(-0.75f, 0.75f)
            + FVector::UpVector * FMath::FRandRange(bMuzzle ? -0.35f : 0.16f, 0.65f);
        const FVector Velocity = (Axis + Scatter).GetSafeNormal()
            * FMath::FRandRange(bSlide ? 70.0f : 130.0f, bMuzzle ? 550.0f : 325.0f);
        Burst->Particles.Add({Velocity, bMuzzle ? (Index == 0 ? 0.48f : 0.22f) : 0.14f,
            bMuzzle ? 0.045f : 0.022f});
        Burst->Meshes[Index]->SetMaterial(0, Burst->Glow);
        Burst->Meshes[Index]->SetVisibility(true);
    }
    Burst->Tick(0.0f);
}

void AArenaCombatEffect::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Age += DeltaTime;
    if (Age >= Duration)
    {
        Destroy();
        return;
    }
    const float Fade = 1.0f - Age / Duration;
    if (Glow)
    {
        Glow->SetVectorParameterValue(TEXT("Color"), Tint * (Fade * Fade * 6.0f));
    }
    for (int32 Index = 0; Index < Particles.Num(); ++Index)
    {
        const FParticle& Particle = Particles[Index];
        const FVector Offset = Particle.Velocity * Age - FVector::UpVector * (0.5f * Gravity * Age * Age);
        const FVector Direction = Particle.Velocity - FVector::UpVector * (Gravity * Age);
        const FVector Scale(Particle.Length * Fade, Particle.Width * Fade, Particle.Width * Fade);
        Meshes[Index]->SetRelativeTransform(FTransform(Direction.Rotation(), Offset, Scale));
    }
}
