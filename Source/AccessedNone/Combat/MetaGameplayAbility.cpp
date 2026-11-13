#include "Combat/MetaGameplayAbility.h"
#include "Combat/MeleeHitboxComponent.h"
#include "Combat/MetaGameplayEffects.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "GameFramework/CharacterMovementComponent.h"

// --- UMetaGameplayAbility ---------------------------------------------------

UMetaGameplayAbility::UMetaGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationBlockedTags.AddTag(MetaTags::State_Stunned);
	ActivationBlockedTags.AddTag(MetaTags::State_Dead);
}

AMetaPlayerCharacter* UMetaGameplayAbility::GetPlayerCharacter() const
{
	return Cast<AMetaPlayerCharacter>(GetAvatarActorFromActorInfo());
}

void UMetaGameplayAbility::SetupPlayerAction(const FGameplayTag& AbilityTag)
{
	SetAssetTags(FGameplayTagContainer(AbilityTag));
	BlockAbilitiesWithTag.AddTag(MetaTags::Ability_Player_LightAttack);
	BlockAbilitiesWithTag.AddTag(MetaTags::Ability_Player_HeavyAttack);
	BlockAbilitiesWithTag.AddTag(MetaTags::Ability_Player_Dodge);
	BlockAbilitiesWithTag.AddTag(MetaTags::Ability_Player_Heal);
	BlockAbilitiesWithTag.AddTag(MetaTags::Ability_Player_ThrowDagger);
}

// --- UGA_MeleeAttack --------------------------------------------------------

void UGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AMetaPlayerCharacter* Character = GetPlayerCharacter();
	if (!Character || !Character->TrySpendStamina(GetParams().StaminaCost))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	AlreadyHit.Reset();
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, GetParams().Windup);
	Wait->OnFinish.AddDynamic(this, &UGA_MeleeAttack::OnActiveStart);
	Wait->ReadyForActivation();
}

void UGA_MeleeAttack::OnActiveStart()
{
	Sweep();
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, GetParams().Active);
	Wait->OnFinish.AddDynamic(this, &UGA_MeleeAttack::OnActiveEnd);
	Wait->ReadyForActivation();
}

void UGA_MeleeAttack::OnActiveEnd()
{
	// shortcut: sweeps at the start and end of the active window only; upgrade to per-tick sweeps if fast targets slip through.
	Sweep();
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, GetParams().Recovery);
	Wait->OnFinish.AddDynamic(this, &UGA_MeleeAttack::OnRecoveryEnd);
	Wait->ReadyForActivation();
}

void UGA_MeleeAttack::OnRecoveryEnd()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_MeleeAttack::Sweep()
{
	AMetaPlayerCharacter* Character = GetPlayerCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Character || !SourceASC)
	{
		return;
	}

	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	const FAttackParams P = GetParams();
	for (AActor* Target : Character->GetHitbox()->QueryHits(Character->GetAttackExtent(), Character->GetAttackOffset()))
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (!TargetASC || AlreadyHit.Contains(Target))
		{
			continue;
		}
		AlreadyHit.Add(Target);

		FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UGE_Damage::StaticClass());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_MotionValue, P.MotionValue);
		Spec.Data->AddDynamicAssetTag(P.DamageType);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
}

UGA_LightAttack::UGA_LightAttack()
{
	SetupPlayerAction(MetaTags::Ability_Player_LightAttack);
}

UGA_MeleeAttack::FAttackParams UGA_LightAttack::GetParams() const
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	return { S.LightMotionValue, S.StaminaCostLight, S.LightWindup, S.LightActive, S.LightRecovery, MetaTags::Damage_Type_Light };
}

UGA_HeavyAttack::UGA_HeavyAttack()
{
	SetupPlayerAction(MetaTags::Ability_Player_HeavyAttack);
}

UGA_MeleeAttack::FAttackParams UGA_HeavyAttack::GetParams() const
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	return { S.HeavyMotionValue, S.StaminaCostHeavy, S.HeavyWindup, S.HeavyActive, S.HeavyRecovery, MetaTags::Damage_Type_Heavy };
}

// --- UGA_Dodge --------------------------------------------------------------

UGA_Dodge::UGA_Dodge()
{
	SetupPlayerAction(MetaTags::Ability_Player_Dodge);
}

void UGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AMetaPlayerCharacter* Character = GetPlayerCharacter();
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	if (!Character || !Character->TrySpendStamina(S.StaminaCostDodge))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	FVector Dir = Character->GetCharacterMovement()->GetLastInputVector().GetSafeNormal2D();
	if (Dir.IsNearlyZero())
	{
		Dir = -Character->GetActorForwardVector(); // backstep
	}
	if (!Character->IsLockedOn())
	{
		Character->SetActorRotation(Dir.Rotation());
	}

	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectToSelf(UGE_IFrame::StaticClass()->GetDefaultObject<UGameplayEffect>(), 1.f, MakeEffectContext(Handle, ActorInfo));

	UAbilityTask_ApplyRootMotionConstantForce* Roll = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, TEXT("Dodge"), Dir, S.DodgeSpeed, S.DodgeDuration, false, nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, S.WalkSpeed, true);
	Roll->OnFinish.AddDynamic(this, &UGA_Dodge::OnDodgeFinished);
	Roll->ReadyForActivation();
}

void UGA_Dodge::OnDodgeFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

// --- UGA_Heal ---------------------------------------------------------------

UGA_Heal::UGA_Heal()
{
	SetupPlayerAction(MetaTags::Ability_Player_Heal);
}

void UGA_Heal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AMetaPlayerCharacter* Character = GetPlayerCharacter();
	if (!Character || !Character->ConsumeHealFlask())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	Character->SetHealing(true);
	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, UAccessedNoneSettings::Get().HealUseSeconds);
	Wait->OnFinish.AddDynamic(this, &UGA_Heal::OnDrinkFinished);
	Wait->ReadyForActivation();
}

void UGA_Heal::OnDrinkFinished()
{
	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectToSelf(UGE_Heal::StaticClass()->GetDefaultObject<UGameplayEffect>(), 1.f, MakeEffectContext(CurrentSpecHandle, CurrentActorInfo));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_Heal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AMetaPlayerCharacter* Character = GetPlayerCharacter())
	{
		Character->SetHealing(false);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
