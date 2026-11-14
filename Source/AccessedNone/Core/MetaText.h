#pragma once

#include "CoreMinimal.h"
#include "Internationalization/StringTable.h"

/** All user-facing text comes from String Table ST_Game (spec 7.4 / chapter 11). */
namespace MetaText
{
	inline const FName TableId(TEXT("/Game/Text/ST_Game.ST_Game"));

	inline FText Get(const TCHAR* Key)
	{
		// The asset must be loaded before the first lookup or the text resolves to <MISSING STRING TABLE ENTRY>.
		static UStringTable* Table = []()
		{
			UStringTable* T = LoadObject<UStringTable>(nullptr, *TableId.ToString());
			if (T) { T->AddToRoot(); }
			return T;
		}();
		(void)Table;
		return FText::FromStringTable(TableId, Key);
	}
}
