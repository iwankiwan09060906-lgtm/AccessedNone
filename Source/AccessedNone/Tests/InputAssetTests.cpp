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
	TestEqual(TEXT("IMC_Gameplay mapping count"), Mappings.Num(), 13);

	auto Find = [&](const TCHAR* ActionName) -> const FEnhancedActionKeyMapping*
	{
		return Mappings.FindByPredicate([ActionName](const FEnhancedActionKeyMapping& M) { return M.Action && M.Action->GetName() == ActionName; });
	};
	auto HasTrigger = [](const FEnhancedActionKeyMapping* M, auto Pred)
	{
		return M && M->Triggers.ContainsByPredicate([&](const UInputTrigger* T) { return Pred(T); });
	};

	// Light/heavy share LMB: no triggers (press → release), the controller times the hold.
	const FEnhancedActionKeyMapping* Attack = Find(TEXT("IA_LightAttack"));
	TestTrue(TEXT("attack = LMB without triggers"), Attack && Attack->Key == EKeys::LeftMouseButton && Attack->Triggers.IsEmpty());
	TestNull(TEXT("no separate heavy mapping"), Find(TEXT("IA_HeavyAttack")));
	TestTrue(TEXT("Dodge (roll) = pressed"), HasTrigger(Find(TEXT("IA_Dodge")), [](const UInputTrigger* T) { return T->IsA<UInputTriggerPressed>(); }));
	TestNull(TEXT("no dash mapping (dash became the roll)"), Find(TEXT("IA_Dash")));
	return true;
}

#endif
