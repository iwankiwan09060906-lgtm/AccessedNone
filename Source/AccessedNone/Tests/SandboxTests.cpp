#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/MetaBossBase.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/BossPatternComponent.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayAbility.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaPlayerController.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * S1 DoD, end-to-end in a running game:
 *   UnrealEditor-Cmd AccessedNone.uproject /Game/Maps/L_Dev_Sandbox -game -nullrhi -ExecCmds="Automation RunTests AccessedNone.Sandbox; Quit"
 * Client context only, so it never runs in the editor-context suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxBossFlowTest, "AccessedNone.Sandbox.BossFlow",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxBossFlowTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AMetaPlayerController> PC;
		TWeakObjectPtr<AMetaPlayerCharacter> Player;
		TWeakObjectPtr<AMetaBossBase> Boss;
		UMetaSettingsSubsystem* Settings() const { return World->GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>(); }
	};
	TSharedRef<FState> S = MakeShared<FState>();

	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World())
		{
			S->World = Ctx.World();
		}
	}
	if (!S->World.IsValid() || !S->World->GetMapName().Contains(TEXT("L_Dev_Sandbox")))
	{
		AddError(TEXT("Run this inside L_Dev_Sandbox with -game"));
		return false;
	}

	S->PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(S->World.Get(), 0));
	S->Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(S->World.Get(), 0));
	TActorIterator<AMetaBossBase> BossIt(S->World.Get());
	S->Boss = BossIt ? *BossIt : nullptr;
	if (!S->PC.IsValid() || !S->Player.IsValid() || !S->Boss.IsValid())
	{
		AddError(TEXT("Sandbox needs a MetaPlayerController, MetaPlayerCharacter and one MetaBossBase"));
		return false;
	}

	const UAccessedNoneSettings& Tuning = UAccessedNoneSettings::Get();

	// 1) Pre-set answer, enter the encounter → Armored, setting overridden.
	S->Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	S->PC->AN_Encounter();
	TestEqual(TEXT("encounter active"), S->World->GetSubsystem<UEncounterSubsystem>()->IsInCombatEncounter(), true);
	TestEqual(TEXT("boss Armored"), S->Boss->GetGimmick()->GetState(), EGimmickState::Armored);
	TestEqual(TEXT("Gamma forced to 0.2"), S->Settings()->GetValue(EMetaSettingKey::Gamma), 0.2f);
	TestEqual(TEXT("boss HP = D x 10"), S->Boss->GetMaxHealth(), Tuning.KnightAttackPower * Tuning.BossHPMultiplier);
	// Freeze boss AI so it cannot kill the player while we test.
	S->Boss->Patterns->Pause();

	// 2) ESC during combat → x0.15, closing restores x1.
	S->PC->ToggleMenu();
	TestEqual(TEXT("menu dilation in combat"), UGameplayStatics::GetGlobalTimeDilation(S->World.Get()), Tuning.EncounterMenuTimeDilation);
	S->PC->ToggleMenu();
	TestEqual(TEXT("dilation restored"), UGameplayStatics::GetGlobalTimeDilation(S->World.Get()), 1.f);

	// 3) Put the player in front of the boss, facing it.
	const FVector BossLoc = S->Boss->GetActorLocation();
	const FVector Stand = BossLoc + S->Boss->GetActorForwardVector() * 150.f;
	S->Player->SetActorLocation(FVector(Stand.X, Stand.Y, S->Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	S->Player->SetActorRotation((BossLoc - Stand).GetSafeNormal2D().Rotation());

	const float SwingTime = Tuning.LightWindup + Tuning.LightActive + Tuning.LightRecovery + 0.25f;
	auto Swing = [S]() { S->Player->ActivateAbility(UGA_LightAttack::StaticClass()); };

	// 4) Armored: one real light attack (GA + hitbox) = 1% damage.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, 0.2f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		const UAccessedNoneSettings& Tuning = UAccessedNoneSettings::Get();
		const float Expected = Tuning.KnightAttackPower * Tuning.BossHPMultiplier - Tuning.KnightAttackPower * (1.f - Tuning.ArmoredDamageReduction);
		TestNearlyEqual(TEXT("armored boss takes 1%"), S->Boss->GetHealth(), Expected, 0.01f);
		S->Settings()->SetValue(EMetaSettingKey::Gamma, 0.95f);
		TestEqual(TEXT("Gamma 0.95 → Breaking"), S->Boss->GetGimmick()->GetState(), EGimmickState::Breaking);
		// Full HP again so "10 hits" is measured from max.
		S->Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), S->Boss->GetMaxHealth());
	}, SwingTime));

	// 5) After BreakingDuration → Vulnerable; settings revert does not matter.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestEqual(TEXT("Vulnerable after breaking"), S->Boss->GetGimmick()->GetState(), EGimmickState::Vulnerable);
		S->Settings()->SetValue(EMetaSettingKey::Gamma, 0.5f);
		S->Boss->Patterns->Pause(); // Vulnerable resumes patterns; keep the boss passive.
	}, S->Boss->GetGimmick()->BreakingDuration + 0.5f));

	// 6) Nine light attacks leave it alive, the tenth kills it.
	for (int32 i = 0; i < 9; ++i)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, i == 0 ? 0.1f : SwingTime));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestTrue(TEXT("alive after 9 hits"), S->Boss->GetHealth() > 0.f);
		TestEqual(TEXT("still latched Vulnerable"), S->Boss->GetGimmick()->GetState(), EGimmickState::Vulnerable);
	}, SwingTime));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestEqual(TEXT("boss Defeated on 10th hit"), S->Boss->GetGimmick()->GetState(), EGimmickState::Defeated);
		TestFalse(TEXT("encounter over"), S->World->GetSubsystem<UEncounterSubsystem>()->IsInCombatEncounter());
	}, SwingTime));
	return true;
}

#endif
