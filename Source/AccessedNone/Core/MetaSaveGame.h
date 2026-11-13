#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "MetaSaveGame.generated.h"

/** Spec 8.10. Written only at breakpoints, never during an encounter. */
UCLASS()
class ACCESSEDNONE_API UMetaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("AccessedNone");
	static constexpr int32 UserIndex = 0;

	UPROPERTY() FName LastBreakpointId;
	UPROPERTY() FName LastLevel;
	UPROPERTY() TSet<FName> DefeatedEncounters;
	UPROPERTY() TMap<FName, int32> DeathCountPerEncounter;
	UPROPERTY() TMap<EMetaSettingKey, float> MetaSettings;
	UPROPERTY() int32 HealFlaskCount = 3;
	UPROPERTY() bool bPossessedStatue = false;
	UPROPERTY() bool bConsoleUnlocked = false;
	UPROPERTY() bool bDebuggerGone = false;
	UPROPERTY() bool bGameCleared = false;
};
