#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "Core/MetaSaveGame.h"
#include "Core/MetaLog.h"
#include "Core/MetaText.h"
#include "HAL/PlatformTime.h"

namespace
{
	constexpr EMetaSettingKey AllKeys[] = {
		EMetaSettingKey::Gamma, EMetaSettingKey::MeshQuality, EMetaSettingKey::StereoBalance, EMetaSettingKey::AspectRatio,
		EMetaSettingKey::MasterVolume, EMetaSettingKey::MotionBlur, EMetaSettingKey::MouseSensitivity, EMetaSettingKey::InvertY };

	bool IsDiscrete(EMetaSettingKey Key)
	{
		return Key == EMetaSettingKey::MeshQuality || Key == EMetaSettingKey::AspectRatio
			|| Key == EMetaSettingKey::MotionBlur || Key == EMetaSettingKey::InvertY;
	}

	FString FormatForLog(EMetaSettingKey Key, float Value)
	{
		if (Key == EMetaSettingKey::Gamma || Key == EMetaSettingKey::MasterVolume)
		{
			return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f));
		}
		return IsDiscrete(Key) ? FString::FromInt(FMath::RoundToInt(Value)) : FString::SanitizeFloat(Value);
	}
}

void UMetaSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetAllToDefaults();
}

void UMetaSettingsSubsystem::Deinitialize()
{
	FTSTicker::RemoveTicker(RestoreTickerHandle);
	RestoreTickerHandle.Reset();
	Super::Deinitialize();
}

void UMetaSettingsSubsystem::ResetAllToDefaults()
{
	for (EMetaSettingKey Key : AllKeys)
	{
		Values.Add(Key, GetDefaultValue(Key));
	}
	ActiveRestores.Reset();
	DecoyValues.Reset();
	DecoyKeyBinds.Reset();
}

float UMetaSettingsSubsystem::GetDefaultValue(EMetaSettingKey Key)
{
	switch (Key)
	{
	case EMetaSettingKey::Gamma:            return 0.5f;
	case EMetaSettingKey::MeshQuality:      return 3.f;
	case EMetaSettingKey::MasterVolume:     return 0.8f;
	case EMetaSettingKey::MouseSensitivity: return 1.f;
	default:                                return 0.f;
	}
}

float UMetaSettingsSubsystem::ClampForKey(EMetaSettingKey Key, float Value)
{
	switch (Key)
	{
	case EMetaSettingKey::MeshQuality:      return FMath::Clamp<float>(FMath::RoundToInt(Value), 0.f, 3.f);
	case EMetaSettingKey::StereoBalance:    return FMath::Clamp(Value, -1.f, 1.f);
	case EMetaSettingKey::MouseSensitivity: return FMath::Clamp(Value, 0.1f, 3.f);
	case EMetaSettingKey::AspectRatio:
	case EMetaSettingKey::MotionBlur:
	case EMetaSettingKey::InvertY:          return FMath::Clamp<float>(FMath::RoundToInt(Value), 0.f, 1.f);
	default:                                return FMath::Clamp(Value, 0.f, 1.f);
	}
}

float UMetaSettingsSubsystem::GetValue(EMetaSettingKey Key) const
{
	const float* Found = Values.Find(Key);
	return Found ? *Found : GetDefaultValue(Key);
}

void UMetaSettingsSubsystem::SetValue(EMetaSettingKey Key, float Value, bool bFromBoss)
{
	if (!bFromBoss)
	{
		// A manual change cancels any running auto-restore on that key.
		ActiveRestores.Remove(Key);
	}

	const float NewValue = ClampForKey(Key, Value);
	const float OldValue = GetValue(Key);
	if (FMath::IsNearlyEqual(OldValue, NewValue, KINDA_SMALL_NUMBER))
	{
		return;
	}

	Values.Add(Key, NewValue);
	UE_LOG(LogMetaSettings, Verbose, TEXT("%s: %f -> %f%s"), *UEnum::GetValueAsString(Key), OldValue, NewValue, bFromBoss ? TEXT(" (boss)") : TEXT(""));
	OnSettingChangedNative.Broadcast(Key, OldValue, NewValue);
	OnSettingChanged.Broadcast(Key, OldValue, NewValue);
}

void UMetaSettingsSubsystem::ResetToEncounterDefault(EMetaSettingKey Key, float DefaultVal, const FString& BossDebugName)
{
	const float OldValue = GetValue(Key);
	ActiveRestores.Remove(Key);
	SetValue(Key, DefaultVal, true);
	if (FMath::IsNearlyEqual(OldValue, GetValue(Key)))
	{
		return; // nothing overwritten (e.g. the 2nd and 3rd boss of a pack forcing the same value)
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Boss"), FText::FromString(BossDebugName));
	Args.Add(TEXT("Setting"), UEnum::GetDisplayValueAsText(Key));
	Args.Add(TEXT("Old"), FText::FromString(FormatForLog(Key, OldValue)));
	Args.Add(TEXT("New"), FText::FromString(FormatForLog(Key, GetValue(Key))));
	const FText Line = FText::Format(MetaText::Get(TEXT("SYS_SETTING_OVERRIDE")), Args);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UHintSubsystem* Hints = GI->GetSubsystem<UHintSubsystem>())
		{
			Hints->PushDebuggerMessage(Line);
		}
	}
}

void UMetaSettingsSubsystem::RestoreOverTime(EMetaSettingKey Key, float TargetVal, float Seconds)
{
	if (Seconds <= 0.f)
	{
		SetValue(Key, TargetVal, true);
		return;
	}

	FRestore& R = ActiveRestores.Add(Key);
	R.From = GetValue(Key);
	R.To = ClampForKey(Key, TargetVal);
	R.StartTime = FPlatformTime::Seconds();
	R.Duration = Seconds;

	if (!RestoreTickerHandle.IsValid())
	{
		RestoreTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UMetaSettingsSubsystem::TickRestores));
	}
}

bool UMetaSettingsSubsystem::TickRestores(float /*DeltaTime*/)
{
	const double Now = FPlatformTime::Seconds();
	for (auto It = ActiveRestores.CreateIterator(); It; ++It)
	{
		const FRestore R = It.Value();
		const EMetaSettingKey Key = It.Key();
		const float Alpha = FMath::Clamp(static_cast<float>((Now - R.StartTime) / R.Duration), 0.f, 1.f);
		if (Alpha >= 1.f)
		{
			It.RemoveCurrent();
		}
		SetValue(Key, FMath::Lerp(R.From, R.To, Alpha), true);
	}

	if (ActiveRestores.IsEmpty())
	{
		RestoreTickerHandle.Reset();
		return false;
	}
	return true;
}

void UMetaSettingsSubsystem::SetMenuLocked(bool bLocked)
{
	if (bMenuLocked != bLocked)
	{
		bMenuLocked = bLocked;
		OnMenuLockChanged.Broadcast(bLocked);
	}
}

void UMetaSettingsSubsystem::WriteToSave(UMetaSaveGame& Save) const
{
	Save.MetaSettings = Values;
}

void UMetaSettingsSubsystem::ReadFromSave(const UMetaSaveGame& Save)
{
	for (const TPair<EMetaSettingKey, float>& Pair : Save.MetaSettings)
	{
		SetValue(Pair.Key, Pair.Value);
	}
}
