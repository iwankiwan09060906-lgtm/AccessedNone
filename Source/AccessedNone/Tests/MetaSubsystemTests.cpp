#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/MetaConsoleSubsystem.h"
#include "Meta/HintSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags MetaTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetaSettingsClampAndBroadcastTest, "AccessedNone.MetaSettings.ClampAndBroadcast", MetaTestFlags)
bool FMetaSettingsClampAndBroadcastTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>();
	UMetaSettingsSubsystem* S = NewObject<UMetaSettingsSubsystem>(GI);
	S->ResetAllToDefaults();

	int32 Broadcasts = 0;
	S->OnSettingChangedNative.AddLambda([&Broadcasts](EMetaSettingKey, float, float) { ++Broadcasts; });

	TestEqual(TEXT("Gamma default"), S->GetValue(EMetaSettingKey::Gamma), 0.5f);
	TestEqual(TEXT("MeshQuality default"), S->GetValue(EMetaSettingKey::MeshQuality), 3.f);
	TestEqual(TEXT("MasterVolume default"), S->GetValue(EMetaSettingKey::MasterVolume), 0.8f);

	S->SetValue(EMetaSettingKey::Gamma, 1.5f);
	TestEqual(TEXT("Gamma clamped to 1"), S->GetValue(EMetaSettingKey::Gamma), 1.f);
	TestEqual(TEXT("one broadcast"), Broadcasts, 1);

	S->SetValue(EMetaSettingKey::Gamma, 1.f);
	S->SetValue(EMetaSettingKey::Gamma, 2.f);
	TestEqual(TEXT("unchanged value does not broadcast"), Broadcasts, 1);

	S->SetValue(EMetaSettingKey::MeshQuality, 7.f);
	TestEqual(TEXT("MeshQuality 7 clamps to current 3, no broadcast"), Broadcasts, 1);

	S->SetValue(EMetaSettingKey::MeshQuality, -2.f);
	TestEqual(TEXT("MeshQuality clamped to 0"), S->GetValue(EMetaSettingKey::MeshQuality), 0.f);
	S->SetValue(EMetaSettingKey::StereoBalance, -3.f);
	TestEqual(TEXT("Balance clamped to -1"), S->GetValue(EMetaSettingKey::StereoBalance), -1.f);
	S->SetValue(EMetaSettingKey::MouseSensitivity, 0.f);
	TestEqual(TEXT("Sensitivity clamped to 0.1"), S->GetValue(EMetaSettingKey::MouseSensitivity), 0.1f);
	S->SetValue(EMetaSettingKey::AspectRatio, 0.6f);
	TestEqual(TEXT("AspectRatio rounds to 1"), S->GetValue(EMetaSettingKey::AspectRatio), 1.f);
	TestEqual(TEXT("four more broadcasts"), Broadcasts, 5);

	S->SetMenuLocked(true);
	TestTrue(TEXT("menu locked"), S->IsMenuLocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetaConsoleParserTest, "AccessedNone.Console.Parser", MetaTestFlags)
bool FMetaConsoleParserTest::RunTest(const FString& Parameters)
{
	const FVector2D Clamp(0.01f, 1.f);
	auto Check = [&](const TCHAR* Input, EMetaConsoleCommand Cmd, bool bOk, const TCHAR* Key, float Value = 0.f)
	{
		const FMetaConsoleResult R = UMetaConsoleSubsystem::Parse(Input, Clamp);
		const FString Ctx = FString::Printf(TEXT("'%s'"), Input);
		TestEqual(Ctx + TEXT(" command"), R.Command, Cmd);
		TestEqual(Ctx + TEXT(" success"), R.bSuccess, bOk);
		TestEqual(Ctx + TEXT(" key"), R.TextKey, FName(Key));
		if (bOk)
		{
			TestEqual(Ctx + TEXT(" value"), R.Value, Value);
		}
		return R;
	};

	Check(TEXT("set gravity 9999"), EMetaConsoleCommand::SetGravity, true, TEXT("CON_GRAVITY_OK"), 9999.f);
	Check(TEXT("   SET   Gravity   9999  "), EMetaConsoleCommand::SetGravity, true, TEXT("CON_GRAVITY_OK"), 9999.f);
	Check(TEXT("set gravity 0"), EMetaConsoleCommand::SetGravity, false, TEXT("CON_GRAVITY_REJECT"));
	Check(TEXT("set gravity -5"), EMetaConsoleCommand::SetGravity, false, TEXT("CON_GRAVITY_REJECT"));
	Check(TEXT("set gravity"), EMetaConsoleCommand::SetGravity, false, TEXT("CON_USAGE"));
	Check(TEXT("set gravity abc"), EMetaConsoleCommand::SetGravity, false, TEXT("CON_USAGE"));
	Check(TEXT("set gravity 1 2"), EMetaConsoleCommand::SetGravity, false, TEXT("CON_USAGE"));

	Check(TEXT("slomo 0.05"), EMetaConsoleCommand::Slomo, true, TEXT("CON_SLOMO_OK"), 0.05f);
	Check(TEXT("SLOMO 5"), EMetaConsoleCommand::Slomo, true, TEXT("CON_SLOMO_OK"), 1.f);
	Check(TEXT("slomo 0.001"), EMetaConsoleCommand::Slomo, true, TEXT("CON_SLOMO_OK"), 0.01f);
	Check(TEXT("slomo -1"), EMetaConsoleCommand::Slomo, true, TEXT("CON_SLOMO_OK"), 0.01f);
	Check(TEXT("slomo"), EMetaConsoleCommand::Slomo, false, TEXT("CON_USAGE"));
	Check(TEXT("slomo fast"), EMetaConsoleCommand::Slomo, false, TEXT("CON_USAGE"));

	Check(TEXT("help"), EMetaConsoleCommand::Help, true, TEXT("CON_HELP"));
	Check(TEXT(" HELP "), EMetaConsoleCommand::Help, true, TEXT("CON_HELP"));
	Check(TEXT("god"), EMetaConsoleCommand::Denied, false, TEXT("CON_DENIED"));
	Check(TEXT("KILL"), EMetaConsoleCommand::Denied, false, TEXT("CON_DENIED"));
	Check(TEXT("kill all"), EMetaConsoleCommand::Denied, false, TEXT("CON_DENIED"));

	TestEqual(TEXT("slomoo suggests slomo"), Check(TEXT("slomoo 0.05"), EMetaConsoleCommand::Unknown, false, TEXT("CON_DID_YOU_MEAN")).Suggestion, FString(TEXT("slomo")));
	TestEqual(TEXT("gravty suggests set gravity"), Check(TEXT("set gravty 9999"), EMetaConsoleCommand::Unknown, false, TEXT("CON_DID_YOU_MEAN")).Suggestion, FString(TEXT("set gravity")));
	TestEqual(TEXT("helo suggests help"), Check(TEXT("helo"), EMetaConsoleCommand::Unknown, false, TEXT("CON_DID_YOU_MEAN")).Suggestion, FString(TEXT("help")));
	Check(TEXT("xyzzy"), EMetaConsoleCommand::Unknown, false, TEXT("CON_UNKNOWN"));

	TestEqual(TEXT("empty input"), UMetaConsoleSubsystem::Parse(TEXT("   "), Clamp).Command, EMetaConsoleCommand::None);

	UMetaConsoleSubsystem* C = NewObject<UMetaConsoleSubsystem>(NewObject<UGameInstance>());
	TestEqual(TEXT("autocomplete sl"), C->Autocomplete(TEXT("sl")), FString(TEXT("slomo ")));
	TestEqual(TEXT("autocomplete SE"), C->Autocomplete(TEXT("SE")), FString(TEXT("set gravity ")));
	TestEqual(TEXT("autocomplete h"), C->Autocomplete(TEXT("h")), FString(TEXT("help")));
	TestEqual(TEXT("autocomplete no match"), C->Autocomplete(TEXT("zz")), FString(TEXT("zz")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHintTiersTest, "AccessedNone.Hint.Tiers", MetaTestFlags)
bool FHintTiersTest::RunTest(const FString& Parameters)
{
	const int32 Expected[][2] = { {0,0}, {1,1}, {2,1}, {3,3}, {4,3}, {5,5}, {7,5}, {8,8}, {20,8} };
	for (const auto& Pair : Expected)
	{
		TestEqual(FString::Printf(TEXT("%d deaths"), Pair[0]), UHintSubsystem::GetTierForDeaths(Pair[0]), Pair[1]);
	}

	UHintSubsystem* H = NewObject<UHintSubsystem>(NewObject<UGameInstance>());
	H->RegisterDeath(TEXT("E1"));
	H->RegisterDeath(TEXT("E1"));
	TestEqual(TEXT("E1 deaths"), H->GetDeathCount(TEXT("E1")), 2);
	TestEqual(TEXT("E2 untouched"), H->GetDeathCount(TEXT("E2")), 0);
	H->MarkEncounterCleared(TEXT("E1"));
	H->RegisterDeath(TEXT("E1"));
	TestEqual(TEXT("cleared encounter stops counting"), H->GetDeathCount(TEXT("E1")), 2);
	return true;
}

#endif
