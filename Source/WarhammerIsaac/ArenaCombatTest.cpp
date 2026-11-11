#include "ArenaPawn.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Components/InputComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaCombatActionRules, "WarhammerIsaac.Combat.ActionRules",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FArenaCombatActionRules::RunTest(const FString& Parameters)
{
    AArenaPawn* Player = nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (UWorld* World = Context.World(); World && World->IsGameWorld())
        {
            if (const APlayerController* Controller = World->GetFirstPlayerController())
            {
                Player = Cast<AArenaPawn>(Controller->GetPawn());
                if (Player)
                {
                    break;
                }
            }
        }
    }
    if (!TestNotNull(TEXT("The arena has a possessed player"), Player))
    {
        return false;
    }

    UInputComponent* Input = NewObject<UInputComponent>(Player);
    Player->SetupPlayerInputComponent(Input);
    auto Press = [this, Input](FName Action, FKey Key)
    {
        for (int32 Index = 0; Index < Input->GetNumActionBindings(); ++Index)
        {
            FInputActionBinding& Binding = Input->GetActionBinding(Index);
            if (Binding.GetActionName() == Action && Binding.KeyEvent == IE_Pressed)
            {
                Binding.ActionDelegate.Execute(Key);
                return;
            }
        }
        AddError(FString::Printf(TEXT("Missing action %s"), *Action.ToString()));
    };

    Player->ResetForRound();
    Press(TEXT("Fire"), EKeys::LeftMouseButton);
    Player->Tick(0.016f);
    const int32 RemainingAmmo = Player->GetMagazineSize() - 1;
    TestEqual(TEXT("A shot consumes one cartridge"), Player->GetAmmo(), RemainingAmmo);
    Press(TEXT("Reload"), EKeys::E);
    TestTrue(TEXT("Manual reload works with ammunition left"), Player->IsReloading());
    Press(TEXT("Dash"), EKeys::SpaceBar);
    Press(TEXT("Fire"), EKeys::LeftMouseButton);
    Player->Tick(0.016f);
    TestFalse(TEXT("Dash cancels the reload"), Player->IsReloading());
    TestEqual(TEXT("Dash blocks the queued shot"), Player->GetAmmo(), RemainingAmmo);
    Press(TEXT("Reload"), EKeys::E);
    TestFalse(TEXT("Reload cannot start during the dash"), Player->IsReloading());
    TestEqual(TEXT("Damage is blocked during dash immunity"),
        Player->TakeDamage(20.0f, FDamageEvent(), nullptr, nullptr), 0.0f);
    TestEqual(TEXT("Projectiles are ignored during dash immunity"),
        CastChecked<UPrimitiveComponent>(Player->GetRootComponent())->GetCollisionResponseToChannel(ECC_GameTraceChannel1),
        ECR_Ignore);
    Player->ResetForRound();
    TestEqual(TEXT("Reset restores ammunition"), Player->GetAmmo(), Player->GetMagazineSize());
    TestEqual(TEXT("Reset clears immunity"),
        Player->TakeDamage(20.0f, FDamageEvent(), nullptr, nullptr), 20.0f);
    Player->ResetForRound();
    return true;
}
#endif
