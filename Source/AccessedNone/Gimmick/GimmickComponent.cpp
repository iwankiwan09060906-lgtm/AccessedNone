#include "Gimmick/GimmickComponent.h"
#include "Gimmick/GimmickCondition.h"
#include "Core/MetaLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

FString UGimmickComponent::GetDebugName() const
{
	return GetOwner() ? GetOwner()->GetClass()->GetName() : GetName();
}

bool UGimmickComponent::AreConditionsSatisfied() const
{
	if (Conditions.IsEmpty())
	{
		return false;
	}
	for (const UGimmickCondition* C : Conditions)
	{
		if (!C || !C->IsSatisfied())
		{
			return false;
		}
	}
	return true;
}

void UGimmickComponent::BeginEncounter()
{
	if (State != EGimmickState::Dormant)
	{
		return;
	}

	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		UMetaSettingsSubsystem* Settings = GI->GetSubsystem<UMetaSettingsSubsystem>();
		for (const TPair<EMetaSettingKey, float>& Reset : SettingsToResetOnStart)
		{
			Settings->ResetToEncounterDefault(Reset.Key, Reset.Value, GetDebugName());
		}
	}

	SetState(EGimmickState::Armored);
	ActivateConditions();
	NotifyConditionChanged();
}

void UGimmickComponent::ResetEncounter()
{
	DeactivateConditions();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BreakingTimer);
	}
	SetState(EGimmickState::Dormant);
}

void UGimmickComponent::NotifyConditionChanged()
{
	const bool bSatisfied = AreConditionsSatisfied();
	if (State == EGimmickState::Armored && bSatisfied)
	{
		SetState(EGimmickState::Breaking);
		if (BreakingDuration > 0.f)
		{
			GetWorld()->GetTimerManager().SetTimer(BreakingTimer, this, &UGimmickComponent::FinishBreaking, BreakingDuration, false);
		}
		else
		{
			FinishBreaking();
		}
	}
	else if (State == EGimmickState::Vulnerable && !bLatchOnceBroken && !bSatisfied)
	{
		SetState(EGimmickState::Armored);
	}
	// Breaking ignores condition changes (spec 8.5).
}

void UGimmickComponent::FinishBreaking()
{
	if (State == EGimmickState::Breaking)
	{
		SetState(EGimmickState::Vulnerable);
	}
}

void UGimmickComponent::NotifyDefeated()
{
	if (State == EGimmickState::Dormant || State == EGimmickState::Defeated)
	{
		return;
	}
	DeactivateConditions();
	GetWorld()->GetTimerManager().ClearTimer(BreakingTimer);
	SetState(EGimmickState::Defeated);
}

void UGimmickComponent::SetHintTier(int32 Tier)
{
	if (HintTier != Tier)
	{
		HintTier = Tier;
		OnHintTierChanged.Broadcast(Tier);
	}
}

void UGimmickComponent::SetState(EGimmickState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const EGimmickState Old = State;
	State = NewState;
	UE_LOG(LogGimmick, Log, TEXT("%s: %s -> %s"), *GetDebugName(), *UEnum::GetValueAsString(Old), *UEnum::GetValueAsString(NewState));
	OnStateChanged.Broadcast(Old, NewState);
}

void UGimmickComponent::ActivateConditions()
{
	for (UGimmickCondition* C : Conditions)
	{
		if (C)
		{
			C->ResetCondition();
			C->Activate(this);
		}
	}
}

void UGimmickComponent::DeactivateConditions()
{
	for (UGimmickCondition* C : Conditions)
	{
		if (C)
		{
			C->Deactivate();
		}
	}
}

void UGimmickComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	DeactivateConditions();
	Super::EndPlay(Reason);
}
