#include "Combat/MetaDamageExecution.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"

namespace
{
	struct FDamageCaptures
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
		DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);

		FDamageCaptures()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMetaCombatAttributeSet, AttackPower, Source, false);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UMetaCombatAttributeSet, DamageReduction, Target, false);
		}
	};

	const FDamageCaptures& Captures()
	{
		static FDamageCaptures C;
		return C;
	}
}

UMetaDamageExecution::UMetaDamageExecution()
{
	RelevantAttributesToCapture.Add(Captures().AttackPowerDef);
	RelevantAttributesToCapture.Add(Captures().DamageReductionDef);
}

void UMetaDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	FGameplayTagContainer SpecTags;
	Spec.GetAllAssetTags(SpecTags);

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// 1) Raw
	float Raw = Spec.GetSetByCallerMagnitude(MetaTags::Data_FixedDamage, false, 0.f);
	if (Raw <= 0.f && SpecTags.HasTagExact(MetaTags::Damage_Type_InstantKill))
	{
		Raw = UAccessedNoneSettings::Get().BossSkillDamage;
	}
	if (Raw <= 0.f)
	{
		float AttackPower = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Captures().AttackPowerDef, EvalParams, AttackPower);
		Raw = AttackPower * Spec.GetSetByCallerMagnitude(MetaTags::Data_MotionValue, false, 1.f);
	}

	// 2) i-frame
	if (TargetASC->HasMatchingGameplayTag(MetaTags::State_IFrame) && !SpecTags.HasTagExact(MetaTags::Damage_IgnoreIFrame))
	{
		return;
	}

	// Breaking transition (spec 3.5): untouchable, no IMMUNE text.
	if (TargetASC->HasMatchingGameplayTag(MetaTags::State_Breaking))
	{
		return;
	}

	// 3) invulnerable
	if (TargetASC->HasMatchingGameplayTag(MetaTags::State_Invulnerable))
	{
		FGameplayEventData Event;
		Event.EventTag = MetaTags::Event_Hit_Immune;
		Event.Instigator = Spec.GetContext().GetInstigator();
		Event.Target = TargetASC->GetAvatarActor();
		Event.EventMagnitude = Raw;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC->GetAvatarActor(), MetaTags::Event_Hit_Immune, Event);
		return;
	}

	// 4) reduction
	float DR = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Captures().DamageReductionDef, EvalParams, DR);
	const float Final = Raw * (1.f - FMath::Clamp(DR, 0.f, 0.99f));

	// Order matters: the attribute set reads raw first, then applies damage + reflect on IncomingDamage.
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UMetaCombatAttributeSet::GetIncomingRawDamageAttribute(), EGameplayModOp::Override, Raw));
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UMetaCombatAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Override, Final));
}
