#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Gimmick/GimmickComponent.h"
#include "Gimmick/GimmickCondition.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/MetaConsoleSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags GimmickTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Standalone game instance (real subsystems) + its world. */
	struct FGameFixture
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;

		FGameFixture()
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->AddToRoot();
			GI->InitializeStandalone();
			World = GI->GetWorld();
		}

		~FGameFixture()
		{
			GI->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			GI->RemoveFromRoot();
		}

		UMetaSettingsSubsystem* Settings() const { return GI->GetSubsystem<UMetaSettingsSubsystem>(); }
		UMetaConsoleSubsystem* Console() const { return GI->GetSubsystem<UMetaConsoleSubsystem>(); }

		UGimmickComponent* SpawnGimmick()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UGimmickComponent* G = NewObject<UGimmickComponent>(Actor);
			G->RegisterComponent();
			return G;
		}

		/** FTimerManager ticks at most once per engine frame, so fake a new frame each call. */
		void AdvanceTimers(float Seconds)
		{
			++GFrameCounter;
			World->GetTimerManager().Tick(Seconds);
		}
	};

	UGimmickCondition_SettingThreshold* GammaAtLeast(UGimmickComponent* G, float Threshold)
	{
		UGimmickCondition_SettingThreshold* C = NewObject<UGimmickCondition_SettingThreshold>(G);
		C->Key = EMetaSettingKey::Gamma;
		C->Threshold = Threshold;
		C->bGreaterOrEqual = true;
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGimmickStateMachineTest, "AccessedNone.Gimmick.StateMachine", GimmickTestFlags)
bool FGimmickStateMachineTest::RunTest(const FString& Parameters)
{
	FGameFixture F;
	UGimmickComponent* G = F.SpawnGimmick();
	G->Conditions.Add(GammaAtLeast(G, 0.95f));
	G->SettingsToResetOnStart.Add(EMetaSettingKey::Gamma, 0.2f);
	G->BreakingDuration = 2.f;

	TestEqual(TEXT("starts Dormant"), G->GetState(), EGimmickState::Dormant);

	// Pre-set answer is overridden on entry (spec 13 / Encounter.PreSetReset).
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	G->BeginEncounter();
	TestEqual(TEXT("Armored after BeginEncounter"), G->GetState(), EGimmickState::Armored);
	TestEqual(TEXT("Gamma forced to 0.2"), F.Settings()->GetValue(EMetaSettingKey::Gamma), 0.2f);

	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.94f);
	TestEqual(TEXT("0.94 not enough"), G->GetState(), EGimmickState::Armored);
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.95f);
	TestEqual(TEXT("Breaking at 0.95"), G->GetState(), EGimmickState::Breaking);

	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.5f);
	TestEqual(TEXT("Breaking ignores revert"), G->GetState(), EGimmickState::Breaking);
	F.AdvanceTimers(1.f);
	TestEqual(TEXT("still Breaking at 1s"), G->GetState(), EGimmickState::Breaking);
	// A timer set during an already-ticked frame starts counting on the next tick, so allow one extra duration.
	F.AdvanceTimers(G->BreakingDuration + 0.5f);
	TestEqual(TEXT("Vulnerable after BreakingDuration"), G->GetState(), EGimmickState::Vulnerable);

	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.1f);
	TestEqual(TEXT("latched Vulnerable"), G->GetState(), EGimmickState::Vulnerable);

	G->NotifyDefeated();
	TestEqual(TEXT("Defeated"), G->GetState(), EGimmickState::Defeated);

	// Death → Dormant → re-entry resets the setting again.
	G->ResetEncounter();
	TestEqual(TEXT("Dormant after reset"), G->GetState(), EGimmickState::Dormant);
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	G->BeginEncounter();
	TestEqual(TEXT("Armored on re-entry"), G->GetState(), EGimmickState::Armored);
	TestEqual(TEXT("Gamma forced again"), F.Settings()->GetValue(EMetaSettingKey::Gamma), 0.2f);

	// Reset mid-Breaking cancels the pending transition.
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	TestEqual(TEXT("Breaking again"), G->GetState(), EGimmickState::Breaking);
	G->ResetEncounter();
	F.AdvanceTimers(5.f);
	TestEqual(TEXT("reset cancels breaking timer"), G->GetState(), EGimmickState::Dormant);

	// Without latch, losing the condition re-arms.
	UGimmickComponent* NoLatch = F.SpawnGimmick();
	NoLatch->Conditions.Add(GammaAtLeast(NoLatch, 0.95f));
	NoLatch->bLatchOnceBroken = false;
	NoLatch->BreakingDuration = 0.f;
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.5f);
	NoLatch->BeginEncounter();
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	TestEqual(TEXT("no-latch Vulnerable"), NoLatch->GetState(), EGimmickState::Vulnerable);
	F.Settings()->SetValue(EMetaSettingKey::Gamma, 0.5f);
	TestEqual(TEXT("no-latch re-arms"), NoLatch->GetState(), EGimmickState::Armored);

	// No conditions = never breaks.
	UGimmickComponent* Plain = F.SpawnGimmick();
	Plain->BeginEncounter();
	TestFalse(TEXT("empty conditions never satisfied"), Plain->AreConditionsSatisfied());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGimmickConsoleAndTest, "AccessedNone.Gimmick.ConsoleAND", GimmickTestFlags)
bool FGimmickConsoleAndTest::RunTest(const FString& Parameters)
{
	auto MakeE6 = [](FGameFixture& F)
	{
		UGimmickComponent* G = F.SpawnGimmick();
		G->BreakingDuration = 0.f;
		UGimmickCondition_ConsoleCommand* Gravity = NewObject<UGimmickCondition_ConsoleCommand>(G);
		Gravity->RequiredCommand = EMetaConsoleCommand::SetGravity;
		Gravity->TargetValue = 9999.f;
		Gravity->Tolerance = 0.f;
		UGimmickCondition_ConsoleCommand* Slomo = NewObject<UGimmickCondition_ConsoleCommand>(G);
		Slomo->RequiredCommand = EMetaConsoleCommand::Slomo;
		Slomo->TargetValue = 0.05f;
		Slomo->Tolerance = 0.001f;
		G->Conditions = { Gravity, Slomo };
		G->BeginEncounter();
		return G;
	};

	for (int32 Order = 0; Order < 2; ++Order)
	{
		FGameFixture F;
		UGimmickComponent* G = MakeE6(F);
		const TCHAR* First = Order == 0 ? TEXT("set gravity 9999") : TEXT("slomo 0.05");
		const TCHAR* Second = Order == 0 ? TEXT("slomo 0.05") : TEXT("set gravity 9999");

		F.Console()->Execute(First);
		TestEqual(FString::Printf(TEXT("order %d: one command is not enough"), Order), G->GetState(), EGimmickState::Armored);
		F.Console()->Execute(TEXT("slomo 0.5"));
		F.Console()->Execute(TEXT("set gravity -1"));
		TestEqual(FString::Printf(TEXT("order %d: wrong values do nothing"), Order), G->GetState(), EGimmickState::Armored);
		F.Console()->Execute(Second);
		TestEqual(FString::Printf(TEXT("order %d: both → Vulnerable"), Order), G->GetState(), EGimmickState::Vulnerable);
	}

	{
		// A new try forgets commands from the previous one.
		FGameFixture F;
		UGimmickComponent* G = MakeE6(F);
		F.Console()->Execute(TEXT("set gravity 9999"));
		G->ResetEncounter();
		G->BeginEncounter();
		F.Console()->Execute(TEXT("slomo 0.05"));
		TestEqual(TEXT("reset clears latched commands"), G->GetState(), EGimmickState::Armored);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSwapAliasesTest, "AccessedNone.Dialogue.SwapAliases", GimmickTestFlags)
bool FDialogueSwapAliasesTest::RunTest(const FString& Parameters)
{
	FGameFixture F;
	UGimmickComponent* G = F.SpawnGimmick();
	G->BreakingDuration = 0.f;
	UGimmickCondition_DialogueSwap* Swap = NewObject<UGimmickCondition_DialogueSwap>(G);
	G->Conditions.Add(Swap);
	G->BeginEncounter();

	// (player tag text, statue tag text, expected)
	struct FCase { const TCHAR* PlayerTag; const TCHAR* StatueTag; bool bOk; };
	const FCase Cases[] = {
		{ TEXT("동상"), TEXT("나"), true },
		{ TEXT(" 동 상 "), TEXT(" 나 "), true },
		{ TEXT("STATUE"), TEXT("ME"), true },
		{ TEXT("statue"), TEXT("Player"), true },
		{ TEXT("동상"), TEXT("p l a y e r"), true },
		{ TEXT("나"), TEXT("동상"), false },
		{ TEXT("동상"), TEXT("용사"), false },
		{ TEXT(""), TEXT("나"), false },
	};
	for (const FCase& C : Cases)
	{
		Swap->EvaluateNames(C.PlayerTag, C.StatueTag);
		TestEqual(FString::Printf(TEXT("player='%s' statue='%s'"), C.PlayerTag, C.StatueTag), Swap->IsSatisfied(), C.bOk);
	}
	TestEqual(TEXT("swap breaks the gimmick"), G->GetState(), EGimmickState::Vulnerable);
	return true;
}

#endif
