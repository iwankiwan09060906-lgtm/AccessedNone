#include "Combat/MetaGameplayEffects.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaDamageExecution.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

void UMetaGameplayEffect::AddModifier(const FGameplayAttribute& Attribute, EGameplayModOp::Type Op, float Value)
{
	FGameplayModifierInfo& Mod = Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = Attribute;
	Mod.ModifierOp = Op;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
}

void UMetaGameplayEffect::GrantTags(std::initializer_list<FGameplayTag> Tags)
{
	UTargetTagsGameplayEffectComponent* Comp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(Comp);
	FInheritedTagContainer Container;
	for (const FGameplayTag& Tag : Tags) { Container.Added.AddTag(Tag); }
	Comp->SetAndApplyTargetTagChanges(Container);
}

void UMetaGameplayEffect::AddAssetTags(std::initializer_list<FGameplayTag> Tags)
{
	UAssetTagsGameplayEffectComponent* Comp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTags"));
	GEComponents.Add(Comp);
	FInheritedTagContainer Container;
	for (const FGameplayTag& Tag : Tags) { Container.Added.AddTag(Tag); }
	Comp->SetAndApplyAssetTagChanges(Container);
}

void UMetaGameplayEffect::MakeInstantDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Executions.AddDefaulted_GetRef().CalculationClass = UMetaDamageExecution::StaticClass();
}

void UMetaGameplayEffect::MakeInitStats(float MaxHP, float Attack, float MaxStamina)
{
	using A = UMetaCombatAttributeSet;
	DurationPolicy = EGameplayEffectDurationType::Instant;
	// Max values first: Health/Stamina are clamped to their max on write.
	AddModifier(A::GetMaxHealthAttribute(), EGameplayModOp::Override, MaxHP);
	AddModifier(A::GetHealthAttribute(), EGameplayModOp::Override, MaxHP);
	AddModifier(A::GetMaxStaminaAttribute(), EGameplayModOp::Override, MaxStamina);
	AddModifier(A::GetStaminaAttribute(), EGameplayModOp::Override, MaxStamina);
	AddModifier(A::GetAttackPowerAttribute(), EGameplayModOp::Override, Attack);
	AddModifier(A::GetDamageReductionAttribute(), EGameplayModOp::Override, 0.f);
	AddModifier(A::GetReflectRatioAttribute(), EGameplayModOp::Override, 0.f);
}

UGE_Boss_Armored::UGE_Boss_Armored()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddModifier(UMetaCombatAttributeSet::GetDamageReductionAttribute(), EGameplayModOp::Override, UAccessedNoneSettings::Get().ArmoredDamageReduction);
	GrantTags({ MetaTags::State_Armored });
}

UGE_Boss_Invulnerable::UGE_Boss_Invulnerable()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	GrantTags({ MetaTags::State_Invulnerable });
}

UGE_Boss_Reflect::UGE_Boss_Reflect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddModifier(UMetaCombatAttributeSet::GetReflectRatioAttribute(), EGameplayModOp::Override, UAccessedNoneSettings::Get().ReflectRatio);
	GrantTags({ MetaTags::Gimmick_Reflect });
}

UGE_Boss_Vulnerable::UGE_Boss_Vulnerable()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddModifier(UMetaCombatAttributeSet::GetDamageReductionAttribute(), EGameplayModOp::Override, 0.f);
	GrantTags({ MetaTags::State_Vulnerable });
}

UGE_Stun::UGE_Stun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = MetaTags::Data_Duration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	GrantTags({ MetaTags::State_Stunned });
}

UGE_IFrame::UGE_IFrame()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(UAccessedNoneSettings::Get().DodgeIFrameSeconds));
	GrantTags({ MetaTags::State_IFrame });
}

UGE_Damage::UGE_Damage()
{
	MakeInstantDamage();
}

UGE_ReflectDamage::UGE_ReflectDamage()
{
	MakeInstantDamage();
	AddAssetTags({ MetaTags::Damage_Type_Reflect });
}

UGE_InstantKill::UGE_InstantKill()
{
	MakeInstantDamage();
	AddAssetTags({ MetaTags::Damage_IgnoreIFrame, MetaTags::Damage_Type_InstantKill });
}

UGE_Heal::UGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FAttributeBasedFloat FromMaxHP;
	FromMaxHP.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(UMetaCombatAttributeSet::GetMaxHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
	FromMaxHP.Coefficient = FScalableFloat(UAccessedNoneSettings::Get().HealPercentOfMaxHP);

	FGameplayModifierInfo& Mod = Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UMetaCombatAttributeSet::GetHealthAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FromMaxHP);
}

UGE_Init_Knight::UGE_Init_Knight()
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	MakeInitStats(S.KnightMaxHealth, S.KnightAttackPower, S.KnightMaxStamina);
}

UGE_Init_Statue::UGE_Init_Statue()
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	MakeInitStats(S.StatueMaxHealth, S.StatueAttackPower, S.StatueMaxStamina);
}
