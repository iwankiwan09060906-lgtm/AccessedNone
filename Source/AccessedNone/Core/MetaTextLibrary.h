#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MetaTextLibrary.generated.h"

class UStringTable;

/** Editor automation hook: Python can't edit String Tables, so Scripts/Editor/create_string_table.py calls this. */
UCLASS()
class ACCESSEDNONE_API UMetaTextLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Replaces Table's entries with the Key,SourceString CSV at FilePath. Returns the entry count, or -1 on failure. */
	UFUNCTION(BlueprintCallable, Category="AccessedNone|Text")
	static int32 ImportStringTableCSV(UStringTable* Table, const FString& FilePath);
};
