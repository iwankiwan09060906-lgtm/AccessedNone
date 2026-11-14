#include "Meta/MetaSaveSubsystem.h"
#include "Core/MetaLog.h"
#include "Core/MetaSaveGame.h"
#include "Meta/HintSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMetaSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Progress = NewObject<UMetaSaveGame>(this);
}

void UMetaSaveSubsystem::StartNewGame()
{
	Progress = NewObject<UMetaSaveGame>(this);
	PendingBreakpoint = NAME_None;
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->ResetAllToDefaults();
	GetGameInstance()->GetSubsystem<UHintSubsystem>()->SetDeathCounts({}, {});
}

FName UMetaSaveSubsystem::LoadForContinue()
{
	UMetaSaveGame* Save = Cast<UMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex));
	if (!Save || Save->LastLevel.IsNone())
	{
		UE_LOG(LogAccessedNone, Warning, TEXT("Continue: no readable save"));
		return NAME_None;
	}
	Progress = Save;
	PendingBreakpoint = Save->LastBreakpointId;
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->ReadFromSave(*Save);
	GetGameInstance()->GetSubsystem<UHintSubsystem>()->SetDeathCounts(Save->DeathCountPerEncounter, Save->DefeatedEncounters);
	return Save->LastLevel;
}

void UMetaSaveSubsystem::SaveAtBreakpoint(FName BreakpointId, UWorld* World)
{
	Progress->LastBreakpointId = BreakpointId;
	Progress->LastLevel = FName(UGameplayStatics::GetCurrentLevelName(World, true));
	Progress->DeathCountPerEncounter = GetGameInstance()->GetSubsystem<UHintSubsystem>()->GetDeathCounts();
	Progress->HealFlaskCount = Progress->GetClass()->GetDefaultObject<UMetaSaveGame>()->HealFlaskCount; // refilled at the breakpoint
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->WriteToSave(*Progress);
	const bool bOk = UGameplayStatics::SaveGameToSlot(Progress, UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex);
	UE_LOG(LogAccessedNone, Log, TEXT("Saved at breakpoint %s in %s (%s)"), *BreakpointId.ToString(), *Progress->LastLevel.ToString(), bOk ? TEXT("ok") : TEXT("FAILED"));
}

void UMetaSaveSubsystem::MarkEncounterDefeated(FName EncounterId)
{
	Progress->DefeatedEncounters.Add(EncounterId);
}

bool UMetaSaveSubsystem::IsEncounterDefeated(FName EncounterId) const
{
	return Progress->DefeatedEncounters.Contains(EncounterId);
}

const TSet<FName>& UMetaSaveSubsystem::GetDefeatedEncounters() const
{
	return Progress->DefeatedEncounters;
}
