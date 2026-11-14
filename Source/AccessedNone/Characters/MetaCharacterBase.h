#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MetaCharacterBase.generated.h"

class UAbilitySystemComponent;
class UMetaCombatAttributeSet;
class UGameplayEffect;
struct FGameplayEventData;

/** Any combatant: owns an ASC + combat attributes, applies InitEffect on BeginPlay, reacts to Event.Death. */
UCLASS(Abstract)
class ACCESSEDNONE_API AMetaCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMetaCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	UMetaCombatAttributeSet* GetAttributes() const { return Attributes; }

	UFUNCTION(BlueprintPure, Category="Combat") float GetHealth() const;
	UFUNCTION(BlueprintPure, Category="Combat") float GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category="Combat") bool IsDead() const;

	/** Paragon anim BPs play an intro montage (LevelStart / SelectScreen) on init whose weapon-summon pose looks broken without its FX. Call after (re)creating the anim instance. */
	void SuppressIntroMontages();

	/**
	 * Plays an attack motion stretched to TotalSeconds (windup + active + recovery), so the swing lands with the hit
	 * of the animation-free combat rules (spec 7.5). Sequences play as a dynamic montage in AttackSlot.
	 */
	void PlayAttackAnim(UAnimSequenceBase* Anim, float TotalSeconds);
	/** Slot for sequence-based attacks; taken from the first montage played. */
	FName AttackSlot = TEXT("DefaultSlot");

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath(const FGameplayEventData* Payload);

	UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnDeath();
	UFUNCTION() void HandleMontageStarted(UAnimMontage* Montage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UMetaCombatAttributeSet> Attributes;

	/** Instant GE applied on BeginPlay to set base stats. */
	UPROPERTY(EditDefaultsOnly, Category="Combat") TSubclassOf<UGameplayEffect> InitEffect;

private:
	double IntroMontageWindowEnd = 0.0;
};
