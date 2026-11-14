#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "UObject/UObjectIterator.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/MetaBossBase.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaGameplayAbility.h"
#include "Combat/MetaGameplayEffects.h"
#include "Meta/EncounterSubsystem.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaLevels.h"
#include "Core/MetaPlayerController.h"
#include "Gimmick/EncounterTriggerBox.h"
#include "Story/PrologueDirector.h"
#include "UI/MetaErrorWindowWidget.h"
#include "UI/MetaTitleWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWorld* GameWorld()
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::Game && Ctx.World())
			{
				return Ctx.World();
			}
		}
		return nullptr;
	}

	bool InMap(const FName Map)
	{
		const UWorld* W = GameWorld();
		return W && W->GetMapName().EndsWith(Map.ToString()) && W->HasBegunPlay();
	}

	APrologueDirector* Director()
	{
		UWorld* W = GameWorld();
		TActorIterator<APrologueDirector> It(W);
		return W && It ? *It : nullptr;
	}

	void Shot(const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FString(TEXT("Opening_")) + Name, true, false); // with UI
	}

	/** Polls Cond each frame; fails the test after Timeout real seconds. */
	void WaitUntil(FAutomationTestBase* Test, const FString& What, double Timeout, TFunction<bool()> Cond)
	{
		TSharedRef<double> Start = MakeShared<double>(-1.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Test, What, Timeout, Cond, Start]()
		{
			if (*Start < 0.0) { *Start = FPlatformTime::Seconds(); }
			if (Cond()) { return true; }
			if (FPlatformTime::Seconds() - *Start > Timeout)
			{
				Test->AddError(TEXT("Timed out waiting for: ") + What);
				return true;
			}
			return false;
		}));
	}

	void StartTrigger(const TCHAR* Id)
	{
		for (TActorIterator<AEncounterTriggerBox> It(GameWorld()); It; ++It)
		{
			if (It->EncounterId == FName(Id)) { It->StartEncounter(); }
		}
	}

	void KillEncounterBosses(const TCHAR* Id)
	{
		AMetaPlayerCharacter* Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GameWorld(), 0));
		UAbilitySystemComponent* Src = Player->GetAbilitySystemComponent();
		for (TActorIterator<AEncounterTriggerBox> It(GameWorld()); It; ++It)
		{
			if (It->EncounterId != FName(Id)) { continue; }
			for (AMetaBossBase* Boss : It->Bosses)
			{
				const FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, Src->MakeEffectContext());
				Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, Boss->GetAbilitySystemComponent());
			}
		}
	}

	void Teleport(const FVector& Location)
	{
		UGameplayStatics::GetPlayerPawn(GameWorld(), 0)->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	}

	void Then(TFunction<void()> Step, float Delay = 0.f)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(MoveTemp(Step), Delay));
	}
}

/**
 * S2 DoD: title → new game → phase 1 clear → phases 2–4 → forced death → error window → Stage 1, in one run.
 *   UnrealEditor AccessedNone.uproject /Game/Maps/L_Title -game -windowed -resx=1280 -resy=720
 *     -ExecCmds="Automation RunTests AccessedNone.Flow.Opening; Quit"
 * Screenshots (when rendering): Saved/Screenshots/.../Opening_*.png
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOpeningFlowTest, "AccessedNone.Flow.Opening", EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FOpeningFlowTest::RunTest(const FString& Parameters)
{
	// Paragon asset-pack Blueprints fail to compile on 5.8 (pre-existing, see DECISIONS.md); not part of this flow.
	AddExpectedError(TEXT("ResetOrientationAndPosition"), EAutomationExpectedErrorFlags::Contains, 0);
	if (!InMap(MetaLevels::Title))
	{
		AddError(TEXT("Start the game in L_Title"));
		return false;
	}

	Then([]() { Shot(TEXT("00_Title")); }, 1.f);
	Then([this]()
	{
		TObjectIterator<UMetaTitleWidget> It;
		if (TestTrue(TEXT("title widget on screen"), static_cast<bool>(It)))
		{
			It->NewGame();
		}
	}, 0.5f);

	WaitUntil(this, TEXT("prologue loaded"), 30.0, []() { return InMap(MetaLevels::Prologue) && Director() && UGameplayStatics::GetPlayerPawn(GameWorld(), 0); });
	Then([]() { Shot(TEXT("01_Fight")); }, 2.f);
	Then([]() { Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(GameWorld(), 0))->AN_Status(); });

	// Attack motions (light combo step, heavy).
	Then([]() { Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GameWorld(), 0))->ActivateAbility(UGA_LightAttack::StaticClass()); });
	Then([]() { Shot(TEXT("01a_LightAttack")); }, 0.3f);
	Then([]() { Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GameWorld(), 0))->ActivateAbility(UGA_HeavyAttack::StaticClass()); }, 1.2f);
	Then([]() { Shot(TEXT("01b_HeavyAttack")); }, 0.6f);

	// Phase 1a: the three fake minions (walking into their trigger).
	Then([this]() { Teleport(FVector(-1500.f, 0.f, 120.f)); StartTrigger(TEXT("P1_Minions")); }, 1.5f);
	Then([]() { Shot(TEXT("02_Minions")); }, 2.5f);
	Then([this]() { KillEncounterBosses(TEXT("P1_Minions")); });
	WaitUntil(this, TEXT("minions cleared"), 5.0, []() { return !GameWorld()->GetSubsystem<UEncounterSubsystem>()->IsInCombatEncounter(); });

	// Walk the avenue (teleport), enter the dungeon: the fake boss is summoned.
	Then([]() { Teleport(FVector(3000.f, 0.f, 120.f)); Shot(TEXT("02a_Avenue")); }, 0.5f);
	Then([]() { Teleport(FVector(6900.f, 0.f, 120.f)); }, 1.5f);
	Then([this]()
	{
		StartTrigger(TEXT("P1"));
		TestTrue(TEXT("boss encounter active"), GameWorld()->GetSubsystem<UEncounterSubsystem>()->GetActiveEncounterId() == FName(TEXT("P1")));
	}, 1.f);
	Then([]() { Shot(TEXT("02b_Summon")); }, 1.0f);
	Then([this]()
	{
		AMetaPlayerController* PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(GameWorld(), 0));
		PC->ToggleMenu();
		TestTrue(TEXT("menu open"), PC->IsMenuOpen());
		TestEqual(TEXT("combat menu dilation"), UGameplayStatics::GetGlobalTimeDilation(GameWorld()), UAccessedNoneSettings::Get().EncounterMenuTimeDilation);
		Shot(TEXT("02c_Settings"));
	}, 1.5f);
	Then([]() { Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(GameWorld(), 0))->ToggleMenu(); }, 1.5f);
	Then([this]() { KillEncounterBosses(TEXT("P1")); }, 0.5f);
	WaitUntil(this, TEXT("DEMON VANQUISHED"), 5.0, []() { return Director() && Director()->GetPhase() == EProloguePhase::Clear; });
	Then([]() { Shot(TEXT("03_Clear")); }, 2.5f);
	WaitUntil(this, TEXT("phase 2"), 10.0, []() { return Director() && Director()->GetPhase() == EProloguePhase::Crack; });
	Then([]() { Shot(TEXT("04_Glitch")); }, 2.3f);
	Then([]() { Shot(TEXT("05_Crack")); }, 3.5f);
	Then([]() { Shot(TEXT("06_Entities")); }, 4.f);
	Then([this]() { TestTrue(TEXT("dozens of warriors falling"), Director()->GetEntityCount() >= 20); }, 1.f);
	WaitUntil(this, TEXT("phase 3"), 10.0, []() { return Director() && Director()->GetPhase() == EProloguePhase::Logs; });
	Then([]() { Shot(TEXT("07_Logs")); }, 8.5f);
	WaitUntil(this, TEXT("phase 4"), 10.0, []() { return Director() && Director()->GetPhase() == EProloguePhase::Execution; });
	Then([]() { Shot(TEXT("08_Overflow")); }, 1.4f);
	Then([]() { Shot(TEXT("09_Explosion")); }, 1.0f);

	// Phase 4 must kill the player within ~5 s no matter what.
	WaitUntil(this, TEXT("forced death"), 6.0, []() { return Director() && Director()->GetPhase() == EProloguePhase::Dead; });
	Then([]() { Shot(TEXT("10_PlayerNone")); }, 3.6f);
	WaitUntil(this, TEXT("error window"), 8.0, []() { return Director() && Director()->GetErrorWindow(); });
	Then([]() { Shot(TEXT("11_ErrorWindow")); }, 2.5f);

	// Phase 5: swap the two numbers.
	Then([this]()
	{
		UMetaErrorWindowWidget* W = Director()->GetErrorWindow();
		TestTrue(TEXT("counter running"), W->GetState().Get(EMetaNumberSlot::Numerator) > 1);
		W->BeginGrab(EMetaNumberSlot::Denominator);
		W->DropOn(EMetaNumberSlot::Numerator);
		TestTrue(TEXT("99999999 / 1"), W->GetState().bSolved && W->GetState().Get(EMetaNumberSlot::Denominator) == 1);
	});
	Then([]() { Shot(TEXT("12_OverflowPopups")); }, 2.f);

	WaitUntil(this, TEXT("Stage 1 loaded"), 30.0, []() { return InMap(MetaLevels::Stage1) && UGameplayStatics::GetPlayerPawn(GameWorld(), 0); });
	Then([this]()
	{
		const AMetaPlayerCharacter* Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(GameWorld(), 0));
		TestTrue(TEXT("respawned alive in Stage 1"), Player && !Player->IsDead());
		Shot(TEXT("13_Stage1"));
	}, 2.f);
	return true;
}

#endif
