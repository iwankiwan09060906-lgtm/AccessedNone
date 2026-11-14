#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MetaSaveSubsystem.generated.h"

class UMetaSaveGame;

/**
 * Session progress + the save slot (spec 8.10). Writes only at breakpoints (never during an encounter).
 * Continue restores settings, hint counters and defeated encounters, then spawns the player at the saved breakpoint.
 */
UCLASS()
class ACCESSEDNONE_API UMetaSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Fresh progress (title → New Game). The slot on disk is kept until the next breakpoint overwrites it. */
	void StartNewGame();
	/** Loads the slot into the session. Returns the level to open, or None when there is no readable save. */
	FName LoadForContinue();
	void SaveAtBreakpoint(FName BreakpointId, UWorld* World);

	void MarkEncounterDefeated(FName EncounterId);
	bool IsEncounterDefeated(FName EncounterId) const;
	const TSet<FName>& GetDefeatedEncounters() const;

	/** Breakpoint to spawn at once (set by Continue, consumed by AMetaGameMode::RestartPlayer). */
	FName ConsumePendingBreakpoint() { const FName Id = PendingBreakpoint; PendingBreakpoint = NAME_None; return Id; }

	UMetaSaveGame* GetProgress() const { return Progress; }

private:
	UPROPERTY() TObjectPtr<UMetaSaveGame> Progress;
	FName PendingBreakpoint;
};
