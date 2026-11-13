#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AccessedNoneSettings.generated.h"

/** All tuning numbers from spec chapter 3 (and 2.2 / 6.2 / 13). Edit in Project Settings or DefaultGame.ini. */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Accessed None Tuning"))
class ACCESSEDNONE_API UAccessedNoneSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UAccessedNoneSettings& Get() { return *GetDefault<UAccessedNoneSettings>(); }

	// --- 3.1 Combat formula ---
	UPROPERTY(Config, EditAnywhere, Category="Combat") float BossHPMultiplier = 10.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float MinionHPMultiplier = 3.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float ArmoredDamageReduction = 0.99f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float ReflectRatio = 3.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float LightMotionValue = 1.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float HeavyMotionValue = 1.5f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float DaggerMotionValue = 1.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat") float DodgeIFrameSeconds = 0.3f;

	// --- 3.3 Boss damage tiers ---
	UPROPERTY(Config, EditAnywhere, Category="Combat|Boss") float BossNormalDamage = 600.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat|Boss") float BossHeavyDamage = 1100.f;
	UPROPERTY(Config, EditAnywhere, Category="Combat|Boss") float BossSkillDamage = 99999.f;

	// --- 3.3 Player base stats ---
	UPROPERTY(Config, EditAnywhere, Category="Player|Knight") float KnightMaxHealth = 1000.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Knight") float KnightAttackPower = 100.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Knight") float KnightMaxStamina = 100.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Statue") float StatueMaxHealth = 3000.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Statue") float StatueAttackPower = 250.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Statue") float StatueMaxStamina = 150.f;

	// --- 3.3 / 3.4 Heal flask ---
	UPROPERTY(Config, EditAnywhere, Category="Player|Heal") int32 HealFlaskMax = 3;
	UPROPERTY(Config, EditAnywhere, Category="Player|Heal") float HealPercentOfMaxHP = 0.6f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Heal") float HealUseSeconds = 1.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Heal") float HealMoveSpeedScale = 0.5f;

	// --- 3.4 Stamina ---
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaCostDodge = 20.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaCostDashPerSecond = 10.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaCostLight = 10.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaCostHeavy = 25.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaCostJump = 10.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaRegenPerSecond = 30.f;
	UPROPERTY(Config, EditAnywhere, Category="Player|Stamina") float StaminaRegenDelay = 0.8f;

	// --- 2.1 / 3.4 Input & lock-on ---
	UPROPERTY(Config, EditAnywhere, Category="Player|Input") float DashHoldThreshold = 0.2f;
	UPROPERTY(Config, EditAnywhere, Category="Player|LockOn", meta=(Units="cm")) float LockOnRadius = 1500.f;
	/** Full cone angle in front of the camera. */
	UPROPERTY(Config, EditAnywhere, Category="Player|LockOn", meta=(Units="deg")) float LockOnConeAngle = 60.f;

	// --- 3.4 Encounter specifics ---
	UPROPERTY(Config, EditAnywhere, Category="Encounter") float FakeBossHPMultiplier = 15.f;
	UPROPERTY(Config, EditAnywhere, Category="Encounter") float FakeBossDamage = 100.f;
	UPROPERTY(Config, EditAnywhere, Category="Encounter") int32 SwarmUnitCount = 5;

	// --- 2.2 Time dilation ---
	UPROPERTY(Config, EditAnywhere, Category="Time") float EncounterMenuTimeDilation = 0.15f;
	UPROPERTY(Config, EditAnywhere, Category="Time") float ConsoleOpenBossDilation = 0.3f;

	// --- 13 Console clamp ---
	UPROPERTY(Config, EditAnywhere, Category="Console") FVector2D SlomoClamp = FVector2D(0.01f, 1.0f);

	// --- E3 aliases (compared trimmed, whitespace-free, case-insensitive) ---
	UPROPERTY(Config, EditAnywhere, Category="Dialogue") TArray<FString> PlayerAliases = { TEXT("나"), TEXT("me"), TEXT("player") };
	UPROPERTY(Config, EditAnywhere, Category="Dialogue") TArray<FString> StatueAliases = { TEXT("동상"), TEXT("statue") };

	// --- 6.2 Respawn ---
	UPROPERTY(Config, EditAnywhere, Category="Respawn") float RespawnMaxSeconds = 3.f;
};
