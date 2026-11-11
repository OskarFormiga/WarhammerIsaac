#include "ArenaGameMode.h"
#include "ArenaEnemy.h"
#include "ArenaPawn.h"
#include "ArenaProjectile.h"
#include "ArenaVisuals.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Canvas.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

namespace
{
    const FVector StartPosition(-300, -250, 84);
    constexpr double RoomSpacing = 3400.0;

    FVector RoomCenter(int32 Index)
    {
        return FVector(Index * RoomSpacing, 0, 0);
    }

    AStaticMeshActor* AddBlock(UWorld& World, UStaticMesh* Cube, const FVector& Center,
        const FVector& Scale, const FLinearColor& Color, bool bCollision = true)
    {
        AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Center, FRotator::ZeroRotator);
        UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetWorldScale3D(Scale);
        Mesh->SetCollisionProfileName(bCollision ? TEXT("BlockAll") : TEXT("NoCollision"));
        ArenaVisuals::Tint(Mesh, Color);
        return Actor;
    }

    APointLight* AddLamp(UWorld& World, const FVector& Position,
        const FLinearColor& Color = FLinearColor(1, 0.43f, 0.14f), float Intensity = 19000)
    {
        APointLight* Light = World.SpawnActor<APointLight>(Position, FRotator::ZeroRotator);
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->GetLightComponent()->SetLightColor(Color);
        Light->GetLightComponent()->SetIntensity(Intensity);
        Cast<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(650);
        Light->GetLightComponent()->SetCastShadows(false);
        return Light;
    }
}

AArenaGameMode::AArenaGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AArenaPawn::StaticClass();
    HUDClass = AArenaHUD::StaticClass();
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    GetWorld()->SpawnActor<APlayerStart>(StartPosition, FRotator::ZeroRotator);
}

void AArenaGameMode::StartPlay()
{
    Super::StartPlay();
    UWorld& World = *GetWorld();
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
    {
        const FLinearColor Floor(0.055f, 0.060f, 0.075f);
        const FLinearColor Stone(0.12f, 0.11f, 0.14f);
        const FLinearColor Trim(0.20f, 0.16f, 0.11f);
        for (int32 Index = 0; Index < RoomCount; ++Index)
        {
            const FVector Center = RoomCenter(Index);
            auto Block = [&](const FVector& Offset, const FVector& Scale, const FLinearColor& Color,
                bool bCollision = true)
            {
                return AddBlock(World, Cube, Center + Offset, Scale, Color, bCollision);
            };
            Block(FVector(0, 0, -20), FVector(28, 28, 0.4), Floor);
            Block(FVector(0, 1380, 180), FVector(28, 0.4, 3.6), Stone);
            Block(FVector(0, -1380, 22), FVector(28, 0.4, 0.44), Trim);
            if (Index == 0)
            {
                Block(FVector(-1380, 0, 36), FVector(0.4, 28, 0.72), Trim);
            }
            if (Index == RoomCount - 1)
            {
                Block(FVector(1380, 0, 180), FVector(0.4, 28, 3.6), Stone);
            }
            for (double Side : {-1.0, 1.0})
            {
                if (Index > 0)
                {
                    Block(FVector(-1380, Side * 820, 36), FVector(0.4, 11.6, 0.72), Trim);
                }
                if (Index < RoomCount - 1)
                {
                    Block(FVector(1380, Side * 820, 180), FVector(0.4, 11.6, 3.6), Stone);
                    Block(FVector(1700, Side * 244, 36), FVector(6, 0.35, 0.72), Trim);
                    Block(FVector(1260, Side * 220, 100), FVector(0.65, 0.55, 2), Trim);
                }
            }
            if (Index < RoomCount - 1)
            {
                Block(FVector(1700, 0, -20), FVector(6, 4.8, 0.4), Floor);
                FExit& Exit = Exits.AddDefaulted_GetRef();
                Exit.Gate = Block(FVector(1380, 0, 140), FVector(0.4, 4.8, 2.8), Trim);
                const FLinearColor Mint(0.08f, 0.85f, 0.46f);
                Exit.Light = AddLamp(World, Center + FVector(1460, 0, 165), Mint, 0);
                for (double X : {1220.0, 1400.0, 1580.0, 1760.0, 1940.0})
                {
                    for (double Side : {-1.0, 1.0})
                    {
                        AActor* Marker = Block(FVector(X, Side * 55, 2),
                            FVector(1.5, 0.18, 0.03), Mint, false);
                        Marker->SetActorRotation(FRotator(0, -Side * 45, 0));
                        Exit.Markers.Add(Marker);
                    }
                }
            }
            for (int32 Line = -5; Line <= 5; ++Line)
            {
                const double Offset = Line * 250.0;
                const FLinearColor Grout(0.08f, 0.09f, 0.11f);
                Block(FVector(Offset, 0, 0.5), FVector(0.025, 27, 0.01), Grout, false);
                Block(FVector(0, Offset, 0.5), FVector(27, 0.025, 0.01), Grout, false);
            }
            const double Side = Index == 1 ? -1.0 : 1.0;
            Block(FVector(350, Side * 250, 65), FVector(1.5, 1.5, 1.3), Stone);
            Block(FVector(-600, Side * 500, 65), FVector(1.5, 1.5, 1.3), Stone);
            AddLamp(World, Center + FVector(1150, -500, 190));
            AddLamp(World, Center + FVector(-500, 1150, 190));
        }
    }
    ADirectionalLight* Key = World.SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-55, -25, 0));
    Key->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Key->GetLightComponent()->SetIntensity(1.5f);
    Key->GetLightComponent()->SetLightColor(FLinearColor(0.72f, 0.81f, 1));
    Key->GetComponent()->SetForwardShadingPriority(1);

    ADirectionalLight* Fill = World.SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-70, 150, 0));
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->GetLightComponent()->SetCastShadows(false);
    Fill->GetLightComponent()->SetIntensity(1.1f);
    Fill->GetComponent()->SetForwardShadingPriority(0);
    ResetArena();
}

FVector AArenaGameMode::CameraRoomCenter(const FVector& Position)
{
    return RoomCenter(FMath::Clamp(FMath::RoundToInt(Position.X / RoomSpacing), 0, RoomCount - 1));
}

bool AArenaGameMode::GetPassageCameraTarget(const FVector& Position, FVector& Target)
{
    const int32 PreviousRoom = FMath::FloorToInt(Position.X / RoomSpacing);
    const double PassageX = Position.X - RoomCenter(PreviousRoom).X;
    constexpr double ExitX = 1380.0;
    constexpr double ArrivalX = RoomSpacing - 600.0;
    if (PreviousRoom < 0 || PreviousRoom >= RoomCount - 1 || PassageX < ExitX || PassageX > ArrivalX)
    {
        return false;
    }

    const double LeadIn = FMath::SmoothStep(ExitX, 1780.0, PassageX);
    const double LeadOut = 1.0 - FMath::SmoothStep(2200.0, ArrivalX, PassageX);
    Target.X += 420.0 * LeadIn * LeadOut;
    return true;
}

void AArenaGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!Exits.IsValidIndex(ActiveRoom) || !bRewardChosen)
    {
        return;
    }
    FExit& Exit = Exits[ActiveRoom];
    const float Progress = FMath::Clamp((GetWorld()->GetTimeSeconds() - Exit.OpeningTime) / 0.65f,
        0.0f, 1.0f);
    const float Lift = FMath::SmoothStep(0.0f, 1.0f, Progress);
    if (AActor* Gate = Exit.Gate.Get(); Gate && !Gate->IsHidden())
    {
        Gate->SetActorLocation(RoomCenter(ActiveRoom) + FVector(1380, 0, 140 + Lift * 310));
        if (Progress >= 1.0f)
        {
            Gate->SetActorEnableCollision(false);
            Gate->SetActorHiddenInGame(true);
        }
    }
    if (APointLight* Light = Exit.Light.Get())
    {
        Light->GetLightComponent()->SetIntensity(52000.0f * Lift);
    }
    const APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    const AArenaPawn* Player = Controller ? Cast<AArenaPawn>(Controller->GetPawn()) : nullptr;
    if (Progress >= 1.0f && Player && !Player->IsDead()
        && Player->GetActorLocation().X > RoomCenter(ActiveRoom + 1).X - 1150.0)
    {
        ++ActiveRoom;
        SpawnEncounter();
    }
}

bool AArenaGameMode::IsRewardAvailable() const
{
    return Exits.IsValidIndex(ActiveRoom) && !bRewardChosen && EnemiesRemaining() == 0;
}

void AArenaGameMode::ChooseReward(AArenaPawn& Player, bool bDamage)
{
    if (Player.IsDead() || !IsRewardAvailable())
    {
        return;
    }
    bRewardChosen = true;
    Player.ApplyReward(bDamage);
    FExit& Exit = Exits[ActiveRoom];
    Exit.OpeningTime = GetWorld()->GetTimeSeconds();
    for (const TWeakObjectPtr<AActor>& Marker : Exit.Markers)
    {
        if (Marker.IsValid())
        {
            Marker->SetActorHiddenInGame(false);
        }
    }
}

void AArenaGameMode::ResetArena()
{
    ActiveRoom = 0;
    for (int32 Index = 0; Index < Exits.Num(); ++Index)
    {
        FExit& Exit = Exits[Index];
        Exit.OpeningTime = -1.0f;
        if (AActor* Door = Exit.Gate.Get())
        {
            Door->SetActorLocation(RoomCenter(Index) + FVector(1380, 0, 140));
            Door->SetActorHiddenInGame(false);
            Door->SetActorEnableCollision(true);
        }
        if (APointLight* Light = Exit.Light.Get())
        {
            Light->GetLightComponent()->SetIntensity(0);
        }
        for (const TWeakObjectPtr<AActor>& Marker : Exit.Markers)
        {
            if (Marker.IsValid())
            {
                Marker->SetActorHiddenInGame(true);
            }
        }
    }
    for (const TWeakObjectPtr<AArenaEnemy>& Enemy : Enemies)
    {
        if (Enemy.IsValid())
        {
            Enemy->Destroy();
        }
    }
    Enemies.Reset();
    for (TActorIterator<AArenaProjectile> Shot(GetWorld()); Shot; ++Shot)
    {
        Shot->Destroy();
    }
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (AArenaPawn* Pawn = Cast<AArenaPawn>(PC->GetPawn()))
        {
            Pawn->SetActorLocation(StartPosition, false, nullptr, ETeleportType::TeleportPhysics);
            Pawn->ResetForRound();
        }
    }
    SpawnEncounter();
}

void AArenaGameMode::SpawnEncounter()
{
    bRewardChosen = false;
    Enemies.Reset();
    const FVector Positions[] = {
        FVector(750, -650, 84), FVector(900, 550, 84),
        FVector(-450, 950, 84), FVector(550, 950, 84),
        FVector(1050, -200, 84)
    };
    const int32 Count = ActiveRoom == RoomCount - 1 ? UE_ARRAY_COUNT(Positions) : 4;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (AArenaEnemy* Enemy = GetWorld()->SpawnActor<AArenaEnemy>(
            RoomCenter(ActiveRoom) + Positions[Index], FRotator::ZeroRotator))
        {
            if (Index == 1 || (ActiveRoom > 0 && Index >= 3))
            {
                Enemy->SetRanged();
            }
            Enemy->SetHealth(EnemyHealth);
            Enemies.Add(Enemy);
        }
    }
}

int32 AArenaGameMode::EnemiesRemaining() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AArenaEnemy>& Enemy : Enemies)
    {
        Count += Enemy.IsValid() ? 1 : 0;
    }
    return Count;
}

void AArenaHUD::DrawHUD()
{
    Super::DrawHUD();
    const AArenaGameMode* Mode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
    const AArenaPawn* Player = Cast<AArenaPawn>(GetOwningPawn());
    if (!Mode || !Player || !Canvas)
    {
        return;
    }

    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    DrawRect(FLinearColor(0.02f, 0.025f, 0.035f, 0.92f), 20, Height - 94, 340, 76);
    DrawRect(FLinearColor(0.2f, 0.04f, 0.04f), 36, Height - 55, 220, 16);
    DrawRect(FLinearColor(0.75f, 0.08f, 0.06f), 36, Height - 55, 220 * Player->GetHealth() / 100, 16);
    DrawText(FString::Printf(TEXT("VIDA %.0f / 100"), Player->GetHealth()),
        FLinearColor::White, 36, Height - 83, nullptr, 1.2f);
    const float Cooldown = Player->DashCooldownRemaining();
    const FString DashText = Cooldown > 0 ? FString::Printf(TEXT("Dash %.1fs"), Cooldown) : TEXT("Dash listo");
    DrawText(DashText, FLinearColor(0.3f, 0.85f, 1), 270, Height - 55);

    const FLinearColor Gold(1.0f, 0.71f, 0.26f);
    const FLinearColor Mint(0.28f, 1.0f, 0.64f);
    const float AmmoLeft = Width - 284;
    DrawRect(FLinearColor(0.02f, 0.025f, 0.035f, 0.92f), AmmoLeft, Height - 118, 264, 100);
    DrawText(FString::Printf(TEXT("ESCOPETA   %d / %d"), Player->GetAmmo(), Player->GetMagazineSize()),
        Gold, AmmoLeft + 14, Height - 108, nullptr, 1.15f);
    const float SlotWidth = 200.0f / FMath::Max(1, Player->GetMagazineSize());
    for (int32 Slot = 0; Slot < Player->GetMagazineSize(); ++Slot)
    {
        const float X = AmmoLeft + 14 + Slot * SlotWidth;
        const bool bLoaded = Slot < Player->GetAmmo();
        DrawRect(bLoaded ? Gold : FLinearColor(0.18f, 0.19f, 0.23f),
            X, Height - 86, SlotWidth - 5, 15);
        if (bLoaded)
        {
            DrawRect(FLinearColor(0.8f, 0.29f, 0.08f), X, Height - 82, SlotWidth - 5, 7);
        }
    }
    if (Player->IsReloading())
    {
        DrawText(TEXT("RECARGANDO"), Mint, AmmoLeft + 14, Height - 67);
        DrawRect(FLinearColor(0.14f, 0.2f, 0.2f), AmmoLeft + 14, Height - 48, 200, 5);
        DrawRect(Mint, AmmoLeft + 14, Height - 48, 200 * Player->GetReloadProgress(), 5);
    }
    else
    {
        DrawText(TEXT("E  Recargar"), FLinearColor::White, AmmoLeft + 14, Height - 64);
    }
    DrawText(FString::Printf(TEXT("Dano x%.2f    Cadencia x%.2f"),
        Player->GetDamageMultiplier(), Player->GetRateMultiplier()), FLinearColor::White,
        AmmoLeft + 14, Height - 38);

    const int32 Remaining = Mode->EnemiesRemaining();
    FString Status;
    if (Remaining > 0)
    {
        Status = FString::Printf(TEXT("Sala %d/%d   |   Enemigos: %d"), Mode->GetRoomNumber(),
            AArenaGameMode::RoomCount, Remaining);
    }
    else
    {
        Status = Mode->GetRoomNumber() == AArenaGameMode::RoomCount
            ? TEXT("RECORRIDO COMPLETADO   |   R para reiniciar")
            : (Mode->IsRewardAvailable() ? TEXT("SALA DESPEJADA   |   Elige una mejora")
                : TEXT("SALA DESPEJADA   |   Sigue las luces verdes"));
    }
    DrawText(Status, FLinearColor(0.92f, 0.78f, 0.5f), 24, 24, nullptr, 1.4f);
    DrawText(TEXT("WASD  Mover    Raton  Apuntar    Clic  Disparar    E  Recargar    Espacio  Dash    R  Reiniciar"),
        FLinearColor::White, 24, 54);
    DrawText(Player->IsCameraFollowing() ? TEXT("Camara en seguimiento  |  F2 zona") : TEXT("Fuera de la zona de seguimiento  |  F2 zona"),
        FLinearColor(0.65f, 0.7f, 0.75f), 24, 78);

    if (Mode->IsRewardAvailable() && !Player->IsDead())
    {
        const float CardWidth = FMath::Min(240.0f, (Width - 48.0f) * 0.5f);
        const float Left = Width * 0.5f - CardWidth - 6;
        const float Top = Height * 0.23f;
        auto DrawCard = [&](float X, const FLinearColor& Accent, const TCHAR* Key,
            const TCHAR* Name, const TCHAR* Description)
        {
            DrawRect(FLinearColor(0.03f, 0.035f, 0.05f, 0.96f), X, Top, CardWidth, 96);
            DrawRect(Accent, X, Top, CardWidth, 4);
            DrawRect(Accent, X + 12, Top + 16, 28, 28);
            DrawText(Key, FLinearColor::Black, X + 21, Top + 21, nullptr, 1.2f);
            DrawText(Name, Accent, X + 50, Top + 18, nullptr, 1.5f);
            DrawText(Description, FLinearColor::White, X + 12, Top + 62);
        };
        DrawCard(Left, Gold, TEXT("1"), TEXT("POTENCIA"), TEXT("+25% dano por perdigon"));
        DrawCard(Left + CardWidth + 12, Mint, TEXT("2"), TEXT("CADENCIA"), TEXT("+30% disparos por segundo"));
    }

    for (TActorIterator<AArenaEnemy> Enemy(GetWorld()); Enemy; ++Enemy)
    {
        const FVector Screen = Project(Enemy->GetActorLocation() + FVector(0, 0, 112));
        if (Screen.Z > 0 && Screen.X > 30 && Screen.X < Width - 30 && Screen.Y > 0 && Screen.Y < Height - 100)
        {
            DrawRect(FLinearColor(0.05f, 0.02f, 0.02f, 0.9f), Screen.X - 27, Screen.Y, 54, 6);
            DrawRect(FLinearColor(0.9f, 0.12f, 0.045f), Screen.X - 27, Screen.Y, 54 * Enemy->GetHealthFraction(), 6);
        }
    }
    if (Player->IsDead())
    {
        DrawRect(FLinearColor(0, 0, 0, 0.65f), Width * 0.5f - 180, Height * 0.5f - 45, 360, 95);
        DrawText(TEXT("HAS CAIDO"), FLinearColor(1, 0.3f, 0.2f), Width * 0.5f - 95, Height * 0.5f - 30, nullptr, 2);
        DrawText(TEXT("Pulsa R para volver a intentarlo"), FLinearColor::White, Width * 0.5f - 100, Height * 0.5f + 12);
    }
}
