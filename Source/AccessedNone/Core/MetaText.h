#pragma once

#include "CoreMinimal.h"

/** All user-facing text comes from String Table ST_Game (spec 7.4 / chapter 11). */
namespace MetaText
{
	inline const FName TableId(TEXT("/Game/Text/ST_Game.ST_Game"));

	inline FText Get(const TCHAR* Key)
	{
		return FText::FromStringTable(TableId, Key);
	}
}
