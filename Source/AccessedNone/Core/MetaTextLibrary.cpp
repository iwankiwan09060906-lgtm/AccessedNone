#include "Core/MetaTextLibrary.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"

int32 UMetaTextLibrary::ImportStringTableCSV(UStringTable* Table, const FString& FilePath)
{
	if (!Table)
	{
		return -1;
	}
	FStringTableRef Mutable = Table->GetMutableStringTable();
	Mutable->ClearSourceStrings();
	if (!Mutable->ImportStringsFromCSVFile(FilePath))
	{
		return -1;
	}
	Table->Modify();
	int32 Count = 0;
	auto Counter = [&Count](const FString&, const FString&) { ++Count; return true; };
	Mutable->EnumerateSourceStrings(TFunctionRef<bool(const FString&, const FString&)>(Counter));
	return Count;
}
