#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/AssassinBoss.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Characters/ThornKnightBoss.h"
#include "Characters/BossDataAsset.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayEffects.h"
#include "Components/StaticMeshComponent.h"
#include "Core/BreakpointActor.h"
#include "Core/MetaGameMode.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaLevels.h"
#include "Core/MetaPlayerController.h"
#include "Core/MetaSaveGame.h"
#include "Core/MetaText.h"
#include "Gimmick/EncounterTriggerBox.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "UI/MetaSettingsWidget.h"
#include "UI/MetaYouDiedWidget.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UnrealClient.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Stage1Test
{
	UWorld* World()
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { return Ctx.World(); }
		}
		return nullptr;
	}
	AMetaPlayerCharacter* Player() { return Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World(), 0)); }
	AMetaPlayerController* PC() { return Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(World(), 0)); }
	UMetaSettingsSubsystem* Settings() { return World()->GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>(); }
	UHintSubsystem* Hints() { return World()->GetGameInstance()->GetSubsystem<UHintSubsystem>(); }
	UEncounterSubsystem* Encounters() { return World()->GetSubsystem<UEncounterSubsystem>(); }

	AEncounterTriggerBox* Trigger(FName Id)
	{
		for (TActorIterator<AEncounterTriggerBox> It(World()); It; ++It) { if (It->EncounterId == Id) { return *It; } }
		return nullptr;
	}
	ABreakpointActor* Breakpoint(FName Id)
	{
		for (TActorIterator<ABreakpointActor> It(World()); It; ++It) { if (It->BreakpointId == Id) { return *It; } }
		return nullptr;
	}
	void Teleport(const FVector& At)
	{
		Player()->SetActorLocation(At, false, nullptr, ETeleportType::TeleportPhysics);
	}
	void Hit(AActor* Source, AActor* Target, float MotionValue)
	{
		UAbilitySystemComponent* Src = Cast<IAbilitySystemInterface>(Source)->GetAbilitySystemComponent();
		FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Src->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_MotionValue, MotionValue);
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, Cast<IAbilitySystemInterface>(Target)->GetAbilitySystemComponent());
	}
	void Kill(AActor* Source, AActor* Target)
	{
		UAbilitySystemComponent* Src = Cast<IAbilitySystemInterface>(Source)->GetAbilitySystemComponent();
		const FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, Src->MakeEffectContext());
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, Cast<IAbilitySystemInterface>(Target)->GetAbilitySystemComponent());
	}
	bool Said(const TCHAR* Key) { return Hints()->GetLastDebuggerMessage().EqualTo(MetaText::Get(Key)); }

	void WaitUntil(FAutomationTestBase* Test, const FString& What, double Timeout, TFunction<bool()> Cond)
	{
		TSharedRef<double> Start = MakeShared<double>(-1.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Test, What, Timeout, Cond, Start]()
		{
			if (*Start < 0.0) { *Start = FPlatformTime::Seconds(); }
			if (Cond()) { return true; }
			if (FPlatformTime::Seconds() - *Start > Timeout) { Test->AddError(TEXT("Timed out: ") + What); return true; }
			return false;
		}));
	}
	void Then(TFunction<void()> Step, float Delay = 0.f)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(MoveTemp(Step), Delay));
	}

	/**
	 * DoD scenario for one encounter: rest at the breakpoint, pre-set answer gets overridden, the fight is unfair,
	 * 8 deaths walk the hint tiers (debugger at 3/5, menu blink at 8) with ≤ 3 s respawns, then the gimmick breaks it.
	 */
	void RunEncounter(FAutomationTestBase* T, FName Id, FName BreakpointId, EMetaSettingKey Key, float Answer, float Forced,
		const TCHAR* Hint3, const TCHAR* Hint5, TFunction<void(AMetaBossBase*)> CheckUnfair, TFunction<void(AMetaBossBase*)> CheckBroken, int32 HitsToKill)
	{
		Then([T, BreakpointId]()
		{
			ABreakpointActor* BP = Breakpoint(BreakpointId);
			if (!T->TestNotNull(TEXT("breakpoint"), BP)) { return; }
			Teleport(BP->GetRespawnTransform().GetLocation());
			Player()->Interact();
			T->TestEqual(TEXT("breakpoint is the respawn point"), World()->GetAuthGameMode<AMetaGameMode>()->GetActiveBreakpoint(), BP);
			const UMetaSaveGame* Save = Cast<UMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex));
			T->TestTrue(TEXT("saved at this breakpoint"), Save && Save->LastBreakpointId == BreakpointId && Save->LastLevel == MetaLevels::Stage1);
		}, 0.5f);

		// Encounter.PreSetReset: the answer chosen beforehand is overridden on entry.
		Then([T, Id, Key, Answer, Forced, CheckUnfair]()
		{
			Settings()->SetValue(Key, Answer);
			Trigger(Id)->StartEncounter();
			AMetaBossBase* Boss = Trigger(Id)->Bosses[0];
			T->TestEqual(TEXT("pre-set answer overridden on entry"), Settings()->GetValue(Key), Forced);
			T->TestEqual(TEXT("armored"), Boss->GetGimmick()->GetState(), EGimmickState::Armored);
			CheckUnfair(Boss);
		}, 0.5f);

		for (int32 Death = 1; Death <= 8; ++Death)
		{
			Then([T, Id, Death]()
			{
				if (!Encounters()->IsInCombatEncounter()) { Trigger(Id)->StartEncounter(); }
				Kill(Trigger(Id)->Bosses[0], Player());
				T->TestTrue(FString::Printf(TEXT("death %d"), Death), Player()->IsDead());
			}, 0.3f);
			Then([T]()
			{
				AMetaGameMode* GM = World()->GetAuthGameMode<AMetaGameMode>();
				T->TestTrue(TEXT("YOU DIED screen with retry / main menu"), GM->GetDeathScreen() && GM->GetDeathScreen()->IsA<UMetaYouDiedWidget>());
				T->TestTrue(TEXT("no auto respawn: still dead until 다시하기"), Player()->IsDead());
				GM->RequestRespawn(); // "다시하기"
			}, 0.5f);
			WaitUntil(T, FString::Printf(TEXT("respawn after death %d (<= 3 s)"), Death), 3.0, []() { return !Player()->IsDead(); });
			Then([T, Id, BreakpointId, Death, Hint3, Hint5]()
			{
				T->TestTrue(TEXT("respawned at the breakpoint"), FVector::Dist2D(Player()->GetActorLocation(), Breakpoint(BreakpointId)->GetRespawnTransform().GetLocation()) < 200.f);
				const int32 Tier = UHintSubsystem::GetTierForDeaths(Death);
				T->TestEqual(FString::Printf(TEXT("hint tier after %d deaths"), Death), Hints()->GetTier(Id), Tier);
				T->TestEqual(TEXT("boss knows the tier"), Trigger(Id)->Bosses[0]->GetGimmick()->GetHintTier(), Tier);
				if (Death == 3) { T->TestTrue(TEXT("debugger metaphor hint at 3"), Said(Hint3)); }
				if (Death == 5) { T->TestTrue(TEXT("debugger menu-path hint at 5"), Said(Hint5)); }
			});
		}

		// Tier 8: the answer row blinks in the settings menu.
		Then([T, Id, Key]()
		{
			Trigger(Id)->StartEncounter();
			PC()->ToggleMenu();
			const UMetaSettingsWidget* Menu = Cast<UMetaSettingsWidget>(PC()->GetMenuWidget());
			T->TestTrue(TEXT("answer row blinks at 8 deaths"), Menu && Menu->GetBlinkingKeys().Contains(Key));
			T->TestTrue(TEXT("answer hidden among many rows / tabs"), Menu && Menu->GetRowCount() >= 60 && Menu->GetTabCount() == 7);
		}, 0.3f);
		Then([T, Id, Key, Answer]()
		{
			PC()->ToggleMenu();
			Settings()->SetValue(Key, Answer); // solve it
			T->TestEqual(TEXT("breaking"), Trigger(Id)->Bosses[0]->GetGimmick()->GetState(), EGimmickState::Breaking);
		}, 0.5f);
		Then([T, Id, CheckBroken, HitsToKill]()
		{
			const auto& Bosses = Trigger(Id)->Bosses;
			for (int32 b = 0; b < Bosses.Num(); ++b)
			{
				AMetaBossBase* Boss = Bosses[b];
				T->TestEqual(TEXT("vulnerable"), Boss->GetGimmick()->GetState(), EGimmickState::Vulnerable);
				CheckBroken(Boss);
				for (int32 i = 0; i < HitsToKill - 1; ++i) { Hit(Player(), Boss, 1.f); }
				T->TestFalse(TEXT("alive one hit short"), Boss->IsDead());
				Hit(Player(), Boss, 1.f);
				T->TestTrue(FString::Printf(TEXT("dies to %d light attacks"), HitsToKill), Boss->IsDead());
				if (b + 1 < Bosses.Num())
				{
					T->TestFalse(TEXT("encounter not cleared while the pack lives"), Encounters()->IsEncounterCleared(Id));
				}
			}
			T->TestTrue(TEXT("encounter cleared"), Encounters()->IsEncounterCleared(Id));
		}, 1.3f);
	}
}

/**
 * Stage 3 DoD in a running game:
 *   UnrealEditor-Cmd AccessedNone.uproject /Game/Maps/L_Stage1_DarkGrove -game -nullrhi -ExecCmds="Automation RunTests AccessedNone.Stage1; Quit"
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStage1EncountersTest, "AccessedNone.Stage1.Encounters", EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FStage1EncountersTest::RunTest(const FString& Parameters)
{
	using namespace Stage1Test;
	if (!World() || !World()->GetMapName().EndsWith(MetaLevels::Stage1.ToString()) || !Trigger(TEXT("E1")) || !Trigger(TEXT("E2")))
	{
		AddError(TEXT("Run this inside L_Stage1_DarkGrove with -game"));
		return false;
	}

	// E1: a pack of three, invisible + invulnerable; gamma >= 0.95 reveals them, a short daze, then melee: 3 light hits each (minion D x 3).
	TestEqual(TEXT("E1 is a pack of three assassins"), Trigger(TEXT("E1"))->Bosses.Num(), 3);
	RunEncounter(this, TEXT("E1"), TEXT("BP_Stage1_Start"), EMetaSettingKey::Gamma, 1.f, 0.2f, TEXT("HINT_E1_3"), TEXT("HINT_E1_5"),
		[this](AMetaBossBase* Boss)
		{
			const AAssassinBoss* A = Cast<AAssassinBoss>(Boss);
			TestTrue(TEXT("assassin stealthed"), A && A->IsStealthed() && !A->GetMesh()->IsVisible());
			const float HP = Boss->GetHealth();
			Hit(Player(), Boss, 1.f);
			TestEqual(TEXT("invulnerable: no damage"), Boss->GetHealth(), HP);
		},
		[this](AMetaBossBase* Boss)
		{
			const AAssassinBoss* A = Cast<AAssassinBoss>(Boss);
			TestTrue(TEXT("revealed"), A && !A->IsStealthed() && A->GetMesh()->IsVisible());
			TestTrue(TEXT("dazzled only briefly, then fights"), A && A->StunSeconds <= 1.5f);
		}, 3);
	Then([this]()
	{
		TestNearlyEqual(TEXT("gamma eased back to 50% over 2 s"), Settings()->GetValue(EMetaSettingKey::Gamma), 0.5f, 0.02f);
		TestTrue(TEXT("debugger E1_CLEAR"), Said(TEXT("E1_CLEAR")));
	}, 2.4f);

	// E2: fast one-shot charges, 99% reduction, lethal thorn counter; mesh quality 0 strips the thorns, 10 light hits.
	RunEncounter(this, TEXT("E2"), TEXT("BP_Stage1_Hall"), EMetaSettingKey::MeshQuality, 0.f, 3.f, TEXT("HINT_E2_3"), TEXT("HINT_E2_5"),
		[this](AMetaBossBase* Boss)
		{
			const AThornKnightBoss* K = Cast<AThornKnightBoss>(Boss);
			TestTrue(TEXT("high-poly thorns"), K && !K->IsLowPoly() && K->GetSpikes()[0]->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
			TestTrue(TEXT("charges fast while armored"), Boss->GetCharacterMovement()->MaxWalkSpeed >= 1500.f);
			bool bOneShot = Boss->GetBossData()->PatternSet_Armored.Num() > 0;
			for (const FBossPattern& P : Boss->GetBossData()->PatternSet_Armored) { bOneShot &= P.FixedDamage >= Player()->GetMaxHealth(); }
			TestTrue(TEXT("every armored swing one-shots the knight"), bOneShot);

			// Thorn counter: give the hero a huge pool so the test survives, then measure the burst.
			UAbilitySystemComponent* PASC = Player()->GetAbilitySystemComponent();
			const float MaxHP = Player()->GetMaxHealth();
			PASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetMaxHealthAttribute(), 1.0e6f);
			PASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), 1.0e6f);
			const float BossHP = Boss->GetHealth();
			const int32 Strikes = K ? K->GetThornStrikeCount() : 0;
			Hit(Player(), Boss, 1.f);
			const float D = Player()->GetAttributes()->GetAttackPower();
			TestNearlyEqual(TEXT("99% reduction"), BossHP - Boss->GetHealth(), D * 0.01f, 0.01f);
			TestTrue(TEXT("hitting it fires the thorns"), K && K->GetThornStrikeCount() == Strikes + 1);
			TestTrue(TEXT("thorn burst is lethal (>= any player HP)"), 1.0e6f - Player()->GetHealth() >= 3000.f);
			PASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetMaxHealthAttribute(), MaxHP);
			PASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), MaxHP);
		},
		[this](AMetaBossBase* Boss)
		{
			const AThornKnightBoss* K = Cast<AThornKnightBoss>(Boss);
			TestTrue(TEXT("low-poly, spike collision off"), K && K->IsLowPoly() && K->GetSpikes()[0]->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestTrue(TEXT("box body shown, real mesh hidden"), K && K->GetBlocks().Num() >= 10 && K->GetBlocks()[0]->IsVisible() && !K->GetMesh()->IsVisible());
			const float PlayerHP = Player()->GetHealth();
			const float BossHP = Boss->GetHealth();
			Hit(Player(), Boss, 1.f);
			TestEqual(TEXT("no thorns once broken"), Player()->GetHealth(), PlayerHP);
			TestTrue(TEXT("normal speed once broken"), Boss->GetCharacterMovement()->MaxWalkSpeed < 1000.f);
			TestNearlyEqual(TEXT("full damage once broken"), BossHP - Boss->GetHealth(), Player()->GetAttributes()->GetAttackPower(), 0.01f);
			Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), Boss->GetMaxHealth());
		}, 10);
	return true;
}


/** Rendered walk-through for visual review (Saved/Screenshots/.../Stage1_*.png). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStage1TourTest, "AccessedNone.Stage1.Tour", EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FStage1TourTest::RunTest(const FString& Parameters)
{
	using namespace Stage1Test;
	if (!World() || !Trigger(TEXT("E1")))
	{
		AddError(TEXT("Run this inside L_Stage1_DarkGrove with -game"));
		return false;
	}
	auto Shot = [](const TCHAR* Name) { Then([Name]() { FScreenshotRequest::RequestScreenshot(FString(TEXT("Stage1_")) + Name, true, false); }); };
	auto Look = [](const FVector& From, const FVector& At)
	{
		Teleport(From);
		const FRotator R = (At - From).GetSafeNormal2D().Rotation();
		Player()->SetActorRotation(R);
		PC()->SetControlRotation(R + FRotator(-8.f, 0.f, 0.f));
	};
	// Visual pass only: the hero can't die, so every shot shows the intended state.
	Then([Look]() { Player()->GetAbilitySystemComponent()->AddLooseGameplayTag(MetaTags::State_Invulnerable);
		Look(Breakpoint(TEXT("BP_Stage1_Start"))->GetRespawnTransform().GetLocation() - FVector(150, 0, 0), FVector(3000, 0, 100)); }, 2.f);
	Shot(TEXT("00_Start"));
	Then([Look]() { Look(FVector(1650, 150, 120), FVector(1800, 300, 150)); }, 1.f);
	Shot(TEXT("01_Stele"));
	Then([]() { Player()->Interact(); }, 0.5f);
	Shot(TEXT("02_SteleText"));
	Then([Look]() { Look(FVector(2700, 0, 120), FVector(3700, 0, 120)); Trigger(TEXT("E1"))->StartEncounter(); }, 1.f);
	Then([](){}, 2.f);
	Shot(TEXT("03_E1_Dark"));
	Then([]() { Settings()->SetValue(EMetaSettingKey::Gamma, 0.97f); }, 0.5f);
	Shot(TEXT("04_E1_Whiteout"));
	Then([](){}, 1.5f);
	Shot(TEXT("05_E1_Revealed"));
	Then([]() { for (AMetaBossBase* B : Trigger(TEXT("E1"))->Bosses) { Kill(Player(), B); Kill(Player(), B); } });
	Then([](){}, 3.f);
	Then([Look]() { Look(FVector(5000, 0, 120), FVector(8000, 0, 100)); }, 0.5f);
	Shot(TEXT("06a_Pass"));
	Then([Look]() { Look(FVector(9900, 0, 120), FVector(10600, 0, 50)); }, 0.5f);
	Shot(TEXT("06_Hall_Debris"));
	Then([Look]() { Look(FVector(11300, 0, 120), FVector(13900, 0, 300)); Trigger(TEXT("E2"))->StartEncounter(); }, 1.f);
	Then([](){}, 2.5f);
	// 3/4 view of the knight wherever it walked to.
	auto LookAtKnight = [Look]() { const FVector K = Trigger(TEXT("E2"))->Bosses[0]->GetActorLocation(); Look(K + FVector(-1000.f, -800.f, 0.f), K); };
	Then(LookAtKnight);
	Shot(TEXT("07_E2_Thorns"));
	Then([]() { Settings()->SetValue(EMetaSettingKey::MeshQuality, 0.f); }, 0.3f);
	Then(LookAtKnight, 1.5f);
	Shot(TEXT("08_E2_LowPoly"));
	Then([]() { PC()->ToggleMenu(); }, 0.5f);
	Then([](){}, 0.5f);
	Shot(TEXT("09_Settings"));
	Then([]() { PC()->ToggleMenu(); }, 0.5f);
	Then([]() { Player()->GetAbilitySystemComponent()->RemoveLooseGameplayTag(MetaTags::State_Invulnerable); Kill(Trigger(TEXT("E2"))->Bosses[0], Player()); }, 0.5f);
	Then([](){}, 2.6f);
	Shot(TEXT("10_YouDied"));
	return true;
}

#endif
