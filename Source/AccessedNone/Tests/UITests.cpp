#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Characters/BossDataAsset.h"
#include "Core/MetaText.h"
#include "UI/MetaErrorWindowState.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags EditorFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	const TOptional<EMetaNumberSlot> ErrWinNumeratorTarget = EMetaNumberSlot::Numerator;
	const TOptional<EMetaNumberSlot> ErrWinDenominatorTarget = EMetaNumberSlot::Denominator;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FErrorWindowSwapRatioTest, "AccessedNone.ErrorWindow.SwapRatio", EditorFlags)
bool FErrorWindowSwapRatioTest::RunTest(const FString&)
{
	FMetaErrorWindowState S;
	TestEqual(TEXT("starts 1 / 99999999"), S.Get(EMetaNumberSlot::Denominator), FMetaErrorWindowState::InitialDenominator);
	S.Tick(3.5);
	TestEqual(TEXT("counts 1 per real second"), S.Get(EMetaNumberSlot::Numerator), int64(4));

	S.BeginGrab(EMetaNumberSlot::Denominator);
	TestEqual(TEXT("grab resets numerator to 1"), S.Get(EMetaNumberSlot::Numerator), int64(1));
	S.Tick(5.0);
	TestEqual(TEXT("counter stopped while held"), S.Get(EMetaNumberSlot::Numerator), int64(1));

	TestTrue(TEXT("drop on the other slot solves"), S.Drop(ErrWinNumeratorTarget));
	TestEqual(TEXT("numerator = 99999999"), S.Get(EMetaNumberSlot::Numerator), FMetaErrorWindowState::InitialDenominator);
	TestEqual(TEXT("denominator = 1"), S.Get(EMetaNumberSlot::Denominator), int64(1));
	TestTrue(TEXT("ratio > 1"), S.Ratio() > 1.0);
	S.Tick(10.0);
	TestEqual(TEXT("solved stays put"), S.Get(EMetaNumberSlot::Numerator), FMetaErrorWindowState::InitialDenominator);

	// Grabbing the numerator side works the same way.
	FMetaErrorWindowState S2;
	S2.Tick(7.0);
	S2.BeginGrab(EMetaNumberSlot::Numerator);
	TestTrue(TEXT("numerator → denominator solves"), S2.Drop(ErrWinDenominatorTarget));
	TestEqual(TEXT("exactly 99999999 / 1"), S2.Ratio(), double(FMetaErrorWindowState::InitialDenominator));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FErrorWindowInvalidDropTest, "AccessedNone.ErrorWindow.InvalidDrop", EditorFlags)
bool FErrorWindowInvalidDropTest::RunTest(const FString&)
{
	FMetaErrorWindowState S;
	S.Tick(5.0);
	S.BeginGrab(EMetaNumberSlot::Numerator);
	TestFalse(TEXT("same slot = snap back"), S.Drop(ErrWinNumeratorTarget));
	TestEqual(TEXT("values unchanged"), S.Get(EMetaNumberSlot::Denominator), FMetaErrorWindowState::InitialDenominator);
	TestTrue(TEXT("counter resumes"), S.IsCounting());
	S.Tick(2.0);
	TestEqual(TEXT("counting again from 1"), S.Get(EMetaNumberSlot::Numerator), int64(3));

	S.BeginGrab(EMetaNumberSlot::Denominator);
	TestFalse(TEXT("outside the window = snap back"), S.Drop({}));
	TestEqual(TEXT("numerator still 1"), S.Get(EMetaNumberSlot::Numerator), int64(1));
	TestEqual(TEXT("denominator intact"), S.Get(EMetaNumberSlot::Denominator), FMetaErrorWindowState::InitialDenominator);
	TestFalse(TEXT("not solved"), S.bSolved);
	TestFalse(TEXT("drop without grab does nothing"), S.Drop(ErrWinDenominatorTarget));
	return true;
}

/** Every MetaText::Get(TEXT("KEY")) in Source/ and every boss DisplayNameKey must exist in ST_Game. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTextAllKeysExistTest, "AccessedNone.Text.AllKeysExist", EditorFlags)
bool FTextAllKeysExistTest::RunTest(const FString&)
{
	const UStringTable* Table = LoadObject<UStringTable>(nullptr, *MetaText::TableId.ToString());
	if (!TestNotNull(TEXT("ST_Game asset (run Scripts/Editor/create_string_table.py)"), Table))
	{
		return false;
	}
	FStringTableConstRef ST = Table->GetStringTable();
	auto Has = [&ST](const FString& Key) { FString Out; return ST->GetSourceString(FTextKey(Key), Out); };

	TSet<FString> Keys;
	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::GameSourceDir(), TEXT("*.cpp"), true, false);
	IFileManager::Get().FindFilesRecursive(Files, *FPaths::GameSourceDir(), TEXT("*.h"), true, false, false);
	// Direct calls plus key-array literals used with MetaText::Get (tutorial/log/tab tables).
	const FRegexPattern Pattern(TEXT("TEXT\\(\"((?:UI|SYS|OP|E[1-6]|HINT|CON|END|SET|TITLE)_[A-Z0-9_]+)\"\\)"));
	for (const FString& File : Files)
	{
		FString Content;
		if (File.Contains(TEXT("/Tests/")) || !FFileHelper::LoadFileToString(Content, *File) || !Content.Contains(TEXT("MetaText")))
		{
			continue;
		}
		FRegexMatcher M(Pattern, Content);
		while (M.FindNext())
		{
			Keys.Add(M.GetCaptureGroup(1));
		}
	}
	TestTrue(TEXT("found code keys"), Keys.Num() > 30);

	TArray<FAssetData> Bosses;
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetsByClass(UBossDataAsset::StaticClass()->GetClassPathName(), Bosses, true);
	for (const FAssetData& A : Bosses)
	{
		if (const UBossDataAsset* Data = Cast<UBossDataAsset>(A.GetAsset()); Data && !Data->DisplayNameKey.IsNone())
		{
			Keys.Add(Data->DisplayNameKey.ToString());
		}
	}

	for (const FString& Key : Keys)
	{
		TestTrue(FString::Printf(TEXT("ST_Game has %s"), *Key), Has(Key));
	}
	return true;
}

#endif
