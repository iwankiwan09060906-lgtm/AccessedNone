#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "MetaSettingsSubsystem.generated.h"

class UMetaSaveGame;

/** Virtual (diegetic) settings. Never touches real OS / GameUserSettings (spec 1.3). */
UENUM(BlueprintType)
enum class EMetaSettingKey : uint8
{
	Gamma,            // 0.0 ~ 1.0 (Default 0.5)
	MeshQuality,      // 0(Low) ~ 3(Epic) (Default 3)
	StereoBalance,    // -1.0(Left) ~ +1.0(Right) (Default 0.0)
	AspectRatio,      // 0 = 16:9, 1 = 4:3 (Default 0)
	MasterVolume,     // 0.0 ~ 1.0 (Default 0.8)
	MotionBlur,       // 0 / 1 (Default 0)
	MouseSensitivity, // 0.1 ~ 3.0 (Default 1.0)
	InvertY           // 0 / 1 (Default 0)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMetaSettingChanged, EMetaSettingKey, Key, float, OldValue, float, NewValue);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMetaSettingChangedNative, EMetaSettingKey /*Key*/, float /*Old*/, float /*New*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuLockChanged, bool, bLocked);

UCLASS()
class ACCESSEDNONE_API UMetaSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable) void SetValue(EMetaSettingKey Key, float Value, bool bFromBoss = false);
	UFUNCTION(BlueprintPure) float GetValue(EMetaSettingKey Key) const;
	UFUNCTION(BlueprintCallable) void ResetToEncounterDefault(EMetaSettingKey Key, float DefaultVal, const FString& BossDebugName);
	/** Real-time interpolation, unaffected by time dilation. */
	UFUNCTION(BlueprintCallable) void RestoreOverTime(EMetaSettingKey Key, float TargetVal, float Seconds);
	UFUNCTION(BlueprintCallable) void SetMenuLocked(bool bLocked);
	UFUNCTION(BlueprintPure) bool IsMenuLocked() const { return bMenuLocked; }

	void WriteToSave(UMetaSaveGame& Save) const;
	void ReadFromSave(const UMetaSaveGame& Save);

	/** Restores all keys to their defaults. Called by Initialize; public for tests. */
	void ResetAllToDefaults();

	static float GetDefaultValue(EMetaSettingKey Key);
	static float ClampForKey(EMetaSettingKey Key, float Value);

	UPROPERTY(BlueprintAssignable) FOnMetaSettingChanged OnSettingChanged;
	FOnMetaSettingChangedNative OnSettingChangedNative;
	UPROPERTY(BlueprintAssignable) FOnMenuLockChanged OnMenuLockChanged;

private:
	struct FRestore
	{
		float From = 0.f;
		float To = 0.f;
		double StartTime = 0.0;
		float Duration = 0.f;
	};

	bool TickRestores(float DeltaTime);

	TMap<EMetaSettingKey, float> Values;
	TMap<EMetaSettingKey, FRestore> ActiveRestores;
	FTSTicker::FDelegateHandle RestoreTickerHandle;
	bool bMenuLocked = false;
};
