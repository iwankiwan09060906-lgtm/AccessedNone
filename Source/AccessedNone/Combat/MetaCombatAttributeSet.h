#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MetaCombatAttributeSet.generated.h"

/** Spec 8.4. Single-player: no replication. */
/** Every applied hit (HUD floating text). bImmune = blocked by State.Invulnerable (spec 3.2 step 3). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMetaDamageNative, AActor* /*Target*/, float /*Damage*/, bool /*bImmune*/);
struct ACCESSEDNONE_API FMetaCombatEvents
{
	static FOnMetaDamageNative OnDamage;
};

UCLASS()
class ACCESSEDNONE_API UMetaCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, Health)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, MaxHealth)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, Stamina)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, MaxStamina)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, AttackPower)
	/** 0.0 ~ 0.99 */
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, DamageReduction)
	/** 0.0 ~ 3.0 */
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData ReflectRatio;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, ReflectRatio)

	/** Meta: final damage written by UMetaDamageExecution. */
	UPROPERTY(BlueprintReadOnly, Category="Meta") FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, IncomingDamage)
	/** Meta: raw (pre-reduction) damage, used for reflect. Always written right before IncomingDamage. */
	UPROPERTY(BlueprintReadOnly, Category="Meta") FGameplayAttributeData IncomingRawDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UMetaCombatAttributeSet, IncomingRawDamage)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	float PendingRawDamage = 0.f;
};
