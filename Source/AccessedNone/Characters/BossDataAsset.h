#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BossDataAsset.generated.h"

class UGameplayEffect;
class UAnimMontage;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FBossPattern> PatternSet_Armored;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FBossPattern> PatternSet_Vulnerable;
};
