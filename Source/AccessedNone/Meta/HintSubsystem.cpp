#include "Meta/HintSubsystem.h"
#include "Core/MetaLog.h"

int32 UHintSubsystem::GetTierForDeaths(int32 DeathCount)
{
	static constexpr int32 Thresholds[] = { 8, 5, 3, 1 };
	for (int32 T : Thresholds)
	{
		if (DeathCount >= T)
		{
			return T;
		}
	}
	return 0;
}

int32 UHintSubsystem::RegisterDeath(FName EncounterId)
{
	if (Cleared.Contains(EncounterId))
	{
		return GetTier(EncounterId);
	}

	const int32 OldTier = GetTier(EncounterId);
	++DeathCounts.FindOrAdd(EncounterId);
	const int32 NewTier = GetTier(EncounterId);
	if (NewTier != OldTier)
	{
		UE_LOG(LogGimmick, Log, TEXT("Hint tier %s: %d -> %d"), *EncounterId.ToString(), OldTier, NewTier);
		OnHintTierChanged.Broadcast(EncounterId, NewTier);
	}
	return NewTier;
}

void UHintSubsystem::MarkEncounterCleared(FName EncounterId)
{
	Cleared.Add(EncounterId);
}

int32 UHintSubsystem::GetDeathCount(FName EncounterId) const
{
	const int32* Found = DeathCounts.Find(EncounterId);
	return Found ? *Found : 0;
}

void UHintSubsystem::PushDebuggerMessage(const FText& Message)
{
	UE_LOG(LogAccessedNone, Log, TEXT("[Debugger] %s"), *Message.ToString());
	OnDebuggerMessage.Broadcast(Message);
}

void UHintSubsystem::SetDeathCounts(const TMap<FName, int32>& InCounts, const TSet<FName>& InCleared)
{
	DeathCounts = InCounts;
	Cleared = InCleared;
}
