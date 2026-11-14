#include "Meta/EncounterSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Core/MetaText.h"
#include "Meta/MetaSaveSubsystem.h"
#include "Core/MetaLog.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

UHintSubsystem* UEncounterSubsystem::GetHints() const
{
	const UGameInstance* GI = GetWorld()->GetGameInstance();
	return GI ? GI->GetSubsystem<UHintSubsystem>() : nullptr;
}

bool UEncounterSubsystem::IsEncounterCleared(FName EncounterId) const
{
	return Cleared.Contains(EncounterId);
}

void UEncounterSubsystem::BeginEncounter(FName EncounterId, AMetaBossBase* Boss)
{
	if (!Boss || Cleared.Contains(EncounterId) || (IsInCombatEncounter() && ActiveId != EncounterId))
	{
		return;
	}
	if (ActiveBosses.Contains(Boss))
	{
		return;
	}

	const bool bFirst = !IsInCombatEncounter();
	ActiveId = EncounterId;
	ActiveBosses.Add(Boss);

	if (const UHintSubsystem* Hints = GetHints())
	{
		Boss->GetGimmick()->SetHintTier(Hints->GetTier(EncounterId));
	}
	Boss->EncounterId = EncounterId;
	Boss->StartEncounter(UGameplayStatics::GetPlayerPawn(this, 0));

	if (bFirst)
	{
		UE_LOG(LogGimmick, Log, TEXT("Encounter %s began"), *EncounterId.ToString());
		OnEncounterBegan.Broadcast(EncounterId);
	}
}

void UEncounterSubsystem::NotifyPlayerDied()
{
	if (!IsInCombatEncounter())
	{
		return;
	}

	const FName Id = ActiveId;
	const int32 Tier = GetHints() ? GetHints()->RegisterDeath(Id) : 0;
	bool bHintSent = false;
	for (const TWeakObjectPtr<AMetaBossBase>& Boss : ActiveBosses)
	{
		if (!Boss.IsValid())
		{
			continue;
		}
		const int32 OldTier = Boss->GetGimmick()->GetHintTier();
		Boss->ResetEncounter();
		Boss->GetGimmick()->SetHintTier(Tier);
		Boss->OnHintTier(Tier);
		// Tiers 3 / 5: the debugger speaks (once per encounter, not per boss).
		const UHintDataAsset* Hint = Boss->GetBossData() ? Boss->GetBossData()->HintAsset.Get() : nullptr;
		const FName Key = !Hint || Tier == OldTier ? NAME_None : Tier == 3 ? Hint->Tier3Key : Tier == 5 ? Hint->Tier5Key : NAME_None;
		if (!bHintSent && !Key.IsNone() && GetHints())
		{
			GetHints()->PushDebuggerMessage(MetaText::Get(*Key.ToString()));
			bHintSent = true;
		}
	}
	ActiveBosses.Reset();
	ActiveId = NAME_None;
	UE_LOG(LogGimmick, Log, TEXT("Encounter %s reset (player died, hint tier %d)"), *Id.ToString(), Tier);
}

void UEncounterSubsystem::NotifyBossDefeated(AMetaBossBase* Boss)
{
	if (!ActiveBosses.Contains(Boss))
	{
		return;
	}
	for (const TWeakObjectPtr<AMetaBossBase>& B : ActiveBosses)
	{
		if (B.IsValid() && B->GetGimmick()->GetState() != EGimmickState::Defeated)
		{
			return;
		}
	}

	const FName Id = ActiveId;
	Cleared.Add(Id);
	ActiveBosses.Reset();
	ActiveId = NAME_None;
	if (UHintSubsystem* Hints = GetHints())
	{
		Hints->MarkEncounterCleared(Id);
	}
	GetWorld()->GetGameInstance()->GetSubsystem<UMetaSaveSubsystem>()->MarkEncounterDefeated(Id);
	Boss->ApplyClearEffects(); // once per encounter, not per boss (E1 pack of three)
	UE_LOG(LogGimmick, Log, TEXT("Encounter %s cleared"), *Id.ToString());
	OnEncounterEnded.Broadcast(Id);
}

TArray<AMetaBossBase*> UEncounterSubsystem::GetActiveBosses() const
{
	TArray<AMetaBossBase*> Out;
	for (const TWeakObjectPtr<AMetaBossBase>& B : ActiveBosses)
	{
		if (B.IsValid())
		{
			Out.Add(B.Get());
		}
	}
	return Out;
}

AMetaBossBase* UEncounterSubsystem::GetPrimaryBoss() const
{
	for (AMetaBossBase* B : GetActiveBosses())
	{
		if (!B->IsDead())
		{
			return B;
		}
	}
	return nullptr;
}

void UEncounterSubsystem::RestartEncounter(FName EncounterId, AMetaBossBase* Boss)
{
	Cleared.Remove(EncounterId);
	BeginEncounter(EncounterId, Boss);
}
