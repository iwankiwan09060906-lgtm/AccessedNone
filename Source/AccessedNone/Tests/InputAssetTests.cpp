#include "Misc/AutomationTest.h"
#include "Core/MetaPlayerController.h"
#include "Core/AccessedNoneSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputTriggers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputAssetsTest, "AccessedNone.Input.Assets",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FInputAssetsTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : { TEXT("IMC_Gameplay"), TEXT("IMC_Menu"), TEXT("IMC_UIManipulation"), TEXT("IMC_Console") })
	{
		TestNotNull(Name, AMetaPlayerController::LoadContext(Name));
	}

	const UInputMappingContext* Gameplay = AMetaPlayerController::LoadContext(TEXT("IMC_Gameplay"));
	if (!Gameplay)
	{
		return false;
	}
	const TArray<FEnhancedActionKeyMapping>& Mappings = Gameplay->GetMappings();
	TestEqual(TEXT("IMC_Gameplay mapping count"), Mappings.Num(), 16);

	const UInputAction* Shift = AMetaPlayerController::LoadAction(TEXT("IA_ModifierShift"));
	const float TapHold = UAccessedNoneSettings::Get().DashHoldThreshold;
	auto Find = [&](const TCHAR* ActionName) -> const FEnhancedActionKeyMapping*
	{
		return Mappings.FindByPredicate([ActionName](const FEnhancedActionKeyMapping& M) { return M.Action && M.Action->GetName() == ActionName; });
	};
	auto HasTrigger = [](const FEnhancedActionKeyMapping* M, auto Pred)
	{
		return M && M->Triggers.ContainsByPredicate([&](const UInputTrigger* T) { return Pred(T); });
	};

	TestTrue(TEXT("Heavy = LMB chorded with Shift"), HasTrigger(Find(TEXT("IA_HeavyAttack")), [&](const UInputTrigger* T)
		{ const UInputTriggerChordAction* C = Cast<UInputTriggerChordAction>(T); return C && C->ChordAction == Shift; }));
	TestTrue(TEXT("Light blocked by Shift"), HasTrigger(Find(TEXT("IA_LightAttack")), [&](const UInputTrigger* T)
		{ const UInputTriggerChordBlocker* C = Cast<UInputTriggerChordBlocker>(T); return C && C->ChordAction == Shift; }));
	TestTrue(TEXT("Dodge = tap"), HasTrigger(Find(TEXT("IA_Dodge")), [&](const UInputTrigger* T)
		{ const UInputTriggerTap* C = Cast<UInputTriggerTap>(T); return C && FMath::IsNearlyEqual(C->TapReleaseTimeThreshold, TapHold); }));
	TestTrue(TEXT("Dash = hold"), HasTrigger(Find(TEXT("IA_Dash")), [&](const UInputTrigger* T)
		{ const UInputTriggerHold* C = Cast<UInputTriggerHold>(T); return C && FMath::IsNearlyEqual(C->HoldTimeThreshold, TapHold); }));
	return true;
}

#endif
