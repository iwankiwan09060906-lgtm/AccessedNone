#include "Gimmick/GimmickCondition.h"
#include "Gimmick/GimmickComponent.h"
#include "Core/AccessedNoneSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// --- base ---

void UGimmickCondition::Activate(UGimmickComponent* InOwner)
{
	Owner = InOwner;
}

void UGimmickCondition::Deactivate()
{
	Owner.Reset();
}

void UGimmickCondition::NotifyOwner()
{
	if (Owner.IsValid())
	{
		Owner->NotifyConditionChanged();
	}
}

UGameInstance* UGimmickCondition::GetGameInstance() const
{
	const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	return World ? World->GetGameInstance() : nullptr;
}

// --- SettingThreshold ---

void UGimmickCondition_SettingThreshold::Activate(UGimmickComponent* InOwner)
{
	Super::Activate(InOwner);
	if (UGameInstance* GI = GetGameInstance())
	{
		Handle = GI->GetSubsystem<UMetaSettingsSubsystem>()->OnSettingChangedNative.AddUObject(this, &UGimmickCondition_SettingThreshold::HandleSettingChanged);
	}
}

void UGimmickCondition_SettingThreshold::Deactivate()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UMetaSettingsSubsystem>()->OnSettingChangedNative.Remove(Handle);
	}
	Handle.Reset();
	Super::Deactivate();
}

bool UGimmickCondition_SettingThreshold::IsSatisfied() const
{
	const UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}
	const float Value = GI->GetSubsystem<UMetaSettingsSubsystem>()->GetValue(Key);
	if (bEquals)
	{
		return FMath::IsNearlyEqual(Value, Threshold, KINDA_SMALL_NUMBER);
	}
	return bGreaterOrEqual ? Value >= Threshold : Value <= Threshold;
}

void UGimmickCondition_SettingThreshold::HandleSettingChanged(EMetaSettingKey ChangedKey, float, float)
{
	if (ChangedKey == Key)
	{
		NotifyOwner();
	}
}

// --- ConsoleCommand ---

void UGimmickCondition_ConsoleCommand::Activate(UGimmickComponent* InOwner)
{
	Super::Activate(InOwner);
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UMetaConsoleSubsystem>()->OnCommandExecuted.AddUniqueDynamic(this, &UGimmickCondition_ConsoleCommand::HandleCommand);
	}
}

void UGimmickCondition_ConsoleCommand::Deactivate()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UMetaConsoleSubsystem>()->OnCommandExecuted.RemoveDynamic(this, &UGimmickCondition_ConsoleCommand::HandleCommand);
	}
	Super::Deactivate();
}

void UGimmickCondition_ConsoleCommand::HandleCommand(const FMetaConsoleResult& Result)
{
	if (Result.bSuccess && Result.Command == RequiredCommand && FMath::Abs(Result.Value - TargetValue) <= Tolerance)
	{
		bExecuted = true;
		NotifyOwner();
	}
}

// --- DialogueSwap ---

bool UGimmickCondition_DialogueSwap::MatchesAlias(const FString& Input, const TArray<FString>& Aliases)
{
	FString Normalized = Input.TrimStartAndEnd().ToLower();
	Normalized.RemoveSpacesInline();
	for (const FString& Alias : Aliases)
	{
		if (Normalized == Alias.ToLower())
		{
			return true;
		}
	}
	return false;
}

void UGimmickCondition_DialogueSwap::EvaluateNames(const FString& PlayerTagName, const FString& TargetTagName)
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	// The statue's tag must now name the player, and the player's tag must name the statue.
	bSwapped = MatchesAlias(TargetTagName, S.PlayerAliases) && MatchesAlias(PlayerTagName, S.StatueAliases);
	NotifyOwner();
}

// --- UIDragDrop ---

void UGimmickCondition_UIDragDrop::SetFraction(double InNumerator, double InDenominator)
{
	Numerator = InNumerator;
	Denominator = InDenominator;
	NotifyOwner();
}
