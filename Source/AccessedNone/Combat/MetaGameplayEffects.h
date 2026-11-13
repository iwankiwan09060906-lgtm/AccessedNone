#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MetaGameplayEffects.generated.h"

// Spec 8.4 GE list, built in C++ (no graph assets). Numbers come from UAccessedNoneSettings at CDO construction.

UCLASS(Abstract)
class ACCESSEDNONE_API UMetaGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	void AddModifier(const FGameplayAttribute& Attribute, EGameplayModOp::Type Op, float Value);
	/** Tags granted to the target while the effect is active. */
	void GrantTags(std::initializer_list<FGameplayTag> Tags);
	/** Tags describing the effect itself (read by the damage execution). */
	void AddAssetTags(std::initializer_list<FGameplayTag> Tags);
	void MakeInstantDamage();
	void MakeInitStats(float MaxHP, float Attack, float MaxStamina);
};

UCLASS(meta=(DisplayName="GE_Boss_Armored"))
class ACCESSEDNONE_API UGE_Boss_Armored : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Boss_Armored(); };

UCLASS(meta=(DisplayName="GE_Boss_Invulnerable"))
class ACCESSEDNONE_API UGE_Boss_Invulnerable : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Boss_Invulnerable(); };

UCLASS(meta=(DisplayName="GE_Boss_Reflect"))
class ACCESSEDNONE_API UGE_Boss_Reflect : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Boss_Reflect(); };

UCLASS(meta=(DisplayName="GE_Boss_Vulnerable"))
class ACCESSEDNONE_API UGE_Boss_Vulnerable : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Boss_Vulnerable(); };

/** Duration via SetByCaller Data.Duration. */
UCLASS(meta=(DisplayName="GE_Stun"))
class ACCESSEDNONE_API UGE_Stun : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Stun(); };

UCLASS(meta=(DisplayName="GE_IFrame"))
class ACCESSEDNONE_API UGE_IFrame : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_IFrame(); };

/** SetByCaller Data.MotionValue (player) or Data.FixedDamage (enemies). */
UCLASS(meta=(DisplayName="GE_Damage"))
class ACCESSEDNONE_API UGE_Damage : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Damage(); };

UCLASS(meta=(DisplayName="GE_ReflectDamage"))
class ACCESSEDNONE_API UGE_ReflectDamage : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_ReflectDamage(); };

UCLASS(meta=(DisplayName="GE_InstantKill"))
class ACCESSEDNONE_API UGE_InstantKill : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_InstantKill(); };

UCLASS(meta=(DisplayName="GE_Heal"))
class ACCESSEDNONE_API UGE_Heal : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Heal(); };

UCLASS(meta=(DisplayName="GE_Init_Knight"))
class ACCESSEDNONE_API UGE_Init_Knight : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Init_Knight(); };

UCLASS(meta=(DisplayName="GE_Init_Statue"))
class ACCESSEDNONE_API UGE_Init_Statue : public UMetaGameplayEffect { GENERATED_BODY() public: UGE_Init_Statue(); };
