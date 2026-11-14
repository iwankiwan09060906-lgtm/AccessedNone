#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "BossDataAsset.generated.h"

class UGameplayEffect;
class UAnimMontage;
class USkeletalMesh;
class UAnimInstance;
class UAnimSequenceBase;

/** One animation-free attack: windup (telegraph glow) → active (hitbox) → recovery (spec 8.6). */
USTRUCT(BlueprintType)
struct FBossPattern
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName PatternId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MinRange = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MaxRange = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Windup = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Active = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Recovery = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cooldown = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Weight = 1.f;
	/** 600 / 1100 / 99999 (spec 3.3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FixedDamage = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bInstantKill = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector HitboxExtent = FVector(150, 150, 100);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector HitboxOffset = FVector(150, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> OptionalMontage;
};

/** DA_Hint_*: debugger lines for death tiers 3 and 5 (spec 6.3 / 11.4). Tier 1 = boss visuals, tier 8 = menu blink. */
UCLASS(BlueprintType)
class ACCESSEDNONE_API UHintDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Tier3Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Tier5Key;
};

UCLASS(BlueprintType)
class ACCESSEDNONE_API UBossDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ST_Game key for the boss bar name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DisplayNameKey;
	/** e.g. BP_ThornKnight_C */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DebugName;
	/** HP = player D × MinionHPMultiplier instead of BossHPMultiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bIsMinion = false;
	/** Override for HP multiplier (> 0), e.g. the fake boss uses FakeBossHPMultiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HPMultiplierOverride = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TSubclassOf<UGameplayEffect>> ArmoredEffects;
	/** Optional visuals applied on BeginPlay, so a boss BP needs no C++ subclass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftClassPtr<UAnimInstance> AnimClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") float MeshScale = 1.f;
	/** Whole-actor scale (capsule, mesh, attack hitboxes and ranges grow together). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") float BodyScale = 1.f;
	/** Played on death (last frame held); the body vanishes VanishDelay seconds after it ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftObjectPtr<UAnimSequenceBase> DeathAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") float VanishDelay = 2.f;
	/** Practice target (L_Dev_Sandbox): never attacks or moves, revives RespawnDelay seconds after vanishing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") bool bTrainingDummy = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float RespawnDelay = 1.5f;
	/** Walk speed per state (cm/s); 0 keeps the character default. E2 charges at the player while armored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float ArmoredMoveSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss") float VulnerableMoveSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FBossPattern> PatternSet_Armored;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FBossPattern> PatternSet_Vulnerable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hints") TObjectPtr<UHintDataAsset> HintAsset;
	/** After defeat: settings eased back (E1 gamma → 0.5 over 2 s, E4 balance → 0 after 3 s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clear") TMap<EMetaSettingKey, float> RestoreSettingsOnDefeat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clear") float RestoreDelay = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clear") float RestoreSeconds = 2.f;
	/** Debugger line when the encounter is cleared (E1_CLEAR). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clear") FName ClearMessageKey;
};
