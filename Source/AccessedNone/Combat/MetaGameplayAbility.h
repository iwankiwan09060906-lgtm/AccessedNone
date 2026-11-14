#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MetaGameplayAbility.generated.h"

class AMetaPlayerCharacter;

/** Base for all project abilities. One action at a time; blocked while stunned or dead. */
UCLASS(Abstract)
class ACCESSEDNONE_API UMetaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UMetaGameplayAbility();

protected:
	AMetaPlayerCharacter* GetPlayerCharacter() const;

	/** Sets this ability's asset tag and blocks every other player action while active. */
	void SetupPlayerAction(const FGameplayTag& AbilityTag);
};

/** Shared windup → active → recovery melee flow (spec 7.5). */
UCLASS(Abstract)
class ACCESSEDNONE_API UGA_MeleeAttack : public UMetaGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	struct FAttackParams
	{
		float MotionValue = 1.f;
		float StaminaCost = 0.f;
		float Windup = 0.f;
		float Active = 0.f;
		float Recovery = 0.f;
		FGameplayTag DamageType;
	};
	virtual FAttackParams GetParams() const PURE_VIRTUAL(UGA_MeleeAttack::GetParams, return {};);
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION() void OnActiveStart();
	UFUNCTION() void OnActiveEnd();
	UFUNCTION() void OnRecoveryEnd();
	void Sweep();

	TArray<TWeakObjectPtr<AActor>> AlreadyHit;
};

UCLASS(meta=(DisplayName="GA_LightAttack"))
class ACCESSEDNONE_API UGA_LightAttack : public UGA_MeleeAttack
{
	GENERATED_BODY()
public:
	UGA_LightAttack();
protected:
	virtual FAttackParams GetParams() const override;
};

UCLASS(meta=(DisplayName="GA_HeavyAttack"))
class ACCESSEDNONE_API UGA_HeavyAttack : public UGA_MeleeAttack
{
	GENERATED_BODY()
public:
	UGA_HeavyAttack();
protected:
	virtual FAttackParams GetParams() const override;
};

UCLASS(meta=(DisplayName="GA_Dodge"))
class ACCESSEDNONE_API UGA_Dodge : public UMetaGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Dodge();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
private:
	UFUNCTION() void OnDodgeFinished();
};

UCLASS(meta=(DisplayName="GA_Heal"))
class ACCESSEDNONE_API UGA_Heal : public UMetaGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_Heal();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
	UFUNCTION() void OnDrinkFinished();
};
