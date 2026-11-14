#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AccessedNoneSettings.generated.h"

/** One selectable player hero (visual + attack reach). Stats stay the knight's (spec 3.3). */
USTRUCT(BlueprintType)
struct FHeroDef
{
	GENERATED_BODY()
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) FName Id;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) TSoftClassPtr<UAnimInstance> AnimClass;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) float MeshScale = 1.f;
	/** Attack hitbox; the archer's long offset is a stand-in until AMetaProjectile (S5-T04). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) FVector AttackExtent = FVector(100.f, 100.f, 90.f);
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) FVector AttackOffset = FVector(120.f, 0.f, 0.f);
	/** Light attack combo, cycled per swing (montages or sequences; sequences play in the montages' slot). */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) TArray<TSoftObjectPtr<UAnimSequenceBase>> LightAttackAnims;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UAnimSequenceBase> HeavyAttackAnim;
};

/** All tuning numbers from spec chapter 3 (and 2.2 / 6.2 / 13). Edit in Project Settings or DefaultGame.ini. */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Accessed None Tuning"))
class ACCESSEDNONE_API UAccessedNoneSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAccessedNoneSettings()
	{
		auto Hero = [](const TCHAR* Name, float Scale, FVector Extent, FVector Offset)
		{
			FHeroDef H;
			H.Id = Name;
			H.Mesh = FSoftObjectPath(FString::Printf(TEXT("/Game/Paragon%s/Characters/Heroes/%s/Meshes/%s.%s"), Name, Name, Name, Name));
			H.AnimClass = FSoftObjectPath(FString::Printf(TEXT("/Game/Paragon%s/Characters/Heroes/%s/%s_AnimBlueprint.%s_AnimBlueprint_C"), Name, Name, Name, Name));
			H.MeshScale = Scale;
			H.AttackExtent = Extent;
			H.AttackOffset = Offset;
			return H;
		};
		// Keys 1-4: warrior, archer (ranged), guard, statue. FengMao's mesh asset is named FengMao_GDC.
		Heroes = { Hero(TEXT("Greystone"), 1.f, FVector(100, 100, 90), FVector(120, 0, 0)),
		           Hero(TEXT("Sparrow"), 1.f, FVector(60, 60, 90), FVector(650, 0, 0)),
		           Hero(TEXT("FengMao"), 1.f, FVector(100, 100, 90), FVector(120, 0, 0)),
		           Hero(TEXT("Terra"), 1.f, FVector(120, 120, 90), FVector(140, 0, 0)) };
		Heroes[2].Mesh = FSoftObjectPath(TEXT("/Game/ParagonFengMao/Characters/Heroes/FengMao/Meshes/FengMao_GDC.FengMao_GDC"));
		// Attack motions (Paragon packs): primary combo montages + a slow/ability sequence as the heavy attack.
		auto Anims = [](FHeroDef& H, std::initializer_list<const TCHAR*> Light, const TCHAR* Heavy)
		{
			const FString Dir = FString::Printf(TEXT("/Game/Paragon%s/Characters/Heroes/%s/Animations/"), *H.Id.ToString(), *H.Id.ToString());
			for (const TCHAR* L : Light) { H.LightAttackAnims.Add(TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Dir + L + TEXT(".") + L))); }
			H.HeavyAttackAnim = FSoftObjectPath(Dir + Heavy + TEXT(".") + Heavy);
		};
		Anims(Heroes[0], { TEXT("Attack_PrimaryA_Montage"), TEXT("Attack_PrimaryB_Montage"), TEXT("Attack_PrimaryC_Montage") }, TEXT("Attack_A_Slow"));
		Anims(Heroes[1], { TEXT("Primary_Fire_Med_Montage") }, TEXT("R_Ability_Slow_Fire"));
		Anims(Heroes[2], { TEXT("Melee_A_Med_Montage"), TEXT("Melee_B_Fast_Montage"), TEXT("Melee_C_Med_Montage") }, TEXT("Primary_D"));
		Anims(Heroes[3], { TEXT("Primary_Melee_A_slow_Montage"), TEXT("Primary_Melee_B_slow_Montage"), TEXT("Primary_Melee_C_slow_Montage") }, TEXT("RMB_melee"));
	}

	static const UAccessedNoneSettings& Get() { return *GetDefault<UAccessedNoneSettings>(); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Heroes") TArray<FHeroDef> Heroes;

	// --- 3.1 Combat formula ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float BossHPMultiplier = 10.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float MinionHPMultiplier = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float ArmoredDamageReduction = 0.99f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float ReflectRatio = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float LightMotionValue = 1.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float HeavyMotionValue = 1.5f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float DaggerMotionValue = 1.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat") float DodgeIFrameSeconds = 0.3f;

	// --- 3.3 Boss damage tiers ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat|Boss") float BossNormalDamage = 600.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat|Boss") float BossHeavyDamage = 1100.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Combat|Boss") float BossSkillDamage = 99999.f;

	// --- 3.3 Player base stats ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Knight") float KnightMaxHealth = 1000.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Knight") float KnightAttackPower = 100.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Knight") float KnightMaxStamina = 100.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Statue") float StatueMaxHealth = 3000.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Statue") float StatueAttackPower = 250.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Statue") float StatueMaxStamina = 150.f;

	// --- 3.3 / 3.4 Heal flask ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Heal") int32 HealFlaskMax = 3;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Heal") float HealPercentOfMaxHP = 0.6f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Heal") float HealUseSeconds = 1.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Heal") float HealMoveSpeedScale = 0.5f;

	// --- 3.4 Stamina ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaCostDodge = 20.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaCostDashPerSecond = 10.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaCostLight = 10.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaCostHeavy = 25.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaCostJump = 10.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaRegenPerSecond = 30.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Stamina") float StaminaRegenDelay = 0.8f;

	// --- 2.1 / 3.4 Input & lock-on ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Input") float DashHoldThreshold = 0.2f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|LockOn", meta=(Units="cm")) float LockOnRadius = 1500.f;
	/** Full cone angle in front of the camera. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|LockOn", meta=(Units="deg")) float LockOnConeAngle = 60.f;

	// --- 7.5 Animation-free combat: player attack timing (windup → active → recovery) ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float LightWindup = 0.25f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float LightActive = 0.15f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float LightRecovery = 0.35f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float HeavyWindup = 0.6f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float HeavyActive = 0.2f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float HeavyRecovery = 0.5f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") FVector PlayerHitboxExtent = FVector(100.f, 100.f, 90.f);
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") FVector PlayerHitboxOffset = FVector(120.f, 0.f, 0.f);
	/** A light attack within this many seconds of the previous one continues the combo. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Attack") float ComboResetSeconds = 1.2f;

	// --- Movement ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Movement", meta=(Units="cm/s")) float WalkSpeed = 500.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Movement", meta=(Units="cm/s")) float DashSpeed = 900.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Movement", meta=(Units="cm/s")) float DodgeSpeed = 1400.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Player|Movement") float DodgeDuration = 0.4f;

	// --- 3.4 Encounter specifics ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Encounter") float FakeBossHPMultiplier = 15.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Encounter") float FakeBossDamage = 100.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Encounter") int32 SwarmUnitCount = 5;

	// --- 2.2 Time dilation ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Time") float EncounterMenuTimeDilation = 0.15f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Time") float ConsoleOpenBossDilation = 0.3f;

	// --- 13 Console clamp ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Console") FVector2D SlomoClamp = FVector2D(0.01f, 1.0f);

	// --- E3 aliases (compared trimmed, whitespace-free, case-insensitive) ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Dialogue") TArray<FString> PlayerAliases = { TEXT("나"), TEXT("me"), TEXT("player") };
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Dialogue") TArray<FString> StatueAliases = { TEXT("동상"), TEXT("statue") };

	// --- 6.2 Respawn ---
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Respawn") float RespawnMaxSeconds = 3.f;
};
