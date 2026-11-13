#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HintSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEncounterHintTierChanged, FName, EncounterId, int32, Tier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDebuggerMessage, const FText&, Message);

/**
 * Per-encounter death counts → hint tiers (spec 6.3), and the single queue feeding WBP_Debugger.
 * Tier values are the death thresholds themselves: 0, 1, 3, 5, 8.
 */
UCLASS()
class ACCESSEDNONE_API UHintSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure) static int32 GetTierForDeaths(int32 DeathCount);

	/** Increments the death count unless the encounter is already cleared. Returns the new tier. */
	UFUNCTION(BlueprintCallable) int32 RegisterDeath(FName EncounterId);
	UFUNCTION(BlueprintCallable) void MarkEncounterCleared(FName EncounterId);
	UFUNCTION(BlueprintPure) int32 GetDeathCount(FName EncounterId) const;
	UFUNCTION(BlueprintPure) int32 GetTier(FName EncounterId) const { return GetTierForDeaths(GetDeathCount(EncounterId)); }

	UFUNCTION(BlueprintCallable) void PushDebuggerMessage(const FText& Message);

	void SetDeathCounts(const TMap<FName, int32>& InCounts, const TSet<FName>& InCleared);
	const TMap<FName, int32>& GetDeathCounts() const { return DeathCounts; }

	UPROPERTY(BlueprintAssignable) FOnEncounterHintTierChanged OnHintTierChanged;
	UPROPERTY(BlueprintAssignable) FOnDebuggerMessage OnDebuggerMessage;

private:
	TMap<FName, int32> DeathCounts;
	TSet<FName> Cleared;
};
