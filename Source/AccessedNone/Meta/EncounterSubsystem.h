#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EncounterSubsystem.generated.h"

class AMetaBossBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncounterEvent, FName, EncounterId);

/**
 * Tracks the active encounter in this world (spec 8.7). One encounter at a time; it may contain several
 * bosses (E4 twins, E5 swarm) and ends when all of them are defeated.
 */
UCLASS()
class ACCESSEDNONE_API UEncounterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Adds Boss to the encounter (starting it if needed). Ignored for cleared encounters. */
	UFUNCTION(BlueprintCallable, Category="Encounter") void BeginEncounter(FName EncounterId, AMetaBossBase* Boss);
	UFUNCTION(BlueprintPure, Category="Encounter") bool IsInCombatEncounter() const { return !ActiveId.IsNone(); }
	UFUNCTION(BlueprintPure, Category="Encounter") FName GetActiveEncounterId() const { return ActiveId; }
	UFUNCTION(BlueprintPure, Category="Encounter") bool IsEncounterCleared(FName EncounterId) const;

	/** Player died: +1 death (hint tier), reset all bosses of the active encounter to Dormant. */
	UFUNCTION(BlueprintCallable, Category="Encounter") void NotifyPlayerDied();
	void NotifyBossDefeated(AMetaBossBase* Boss);

	UPROPERTY(BlueprintAssignable) FOnEncounterEvent OnEncounterBegan;
	UPROPERTY(BlueprintAssignable) FOnEncounterEvent OnEncounterEnded;

private:
	class UHintSubsystem* GetHints() const;

	FName ActiveId;
	TArray<TWeakObjectPtr<AMetaBossBase>> ActiveBosses;
	TSet<FName> Cleared;
};
