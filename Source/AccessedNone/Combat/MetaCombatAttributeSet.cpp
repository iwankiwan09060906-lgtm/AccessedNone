#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaLog.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemBlueprintLibrary.h"

void UMetaCombatAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetDamageReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.99f);
	}
	else if (Attribute == GetReflectRatioAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 3.f);
	}
}

void UMetaCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	ClampAttribute(Attribute, NewValue);
}

void UMetaCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	ClampAttribute(Attribute, NewValue);
}

void UMetaCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& Attr = Data.EvaluatedData.Attribute;

	if (Attr == GetIncomingRawDamageAttribute())
	{
		PendingRawDamage = GetIncomingRawDamage();
		SetIncomingRawDamage(0.f);
		return;
	}

	if (Attr != GetIncomingDamageAttribute())
	{
		return;
	}

	const float Damage = GetIncomingDamage();
	const float Raw = PendingRawDamage;
	SetIncomingDamage(0.f);
	PendingRawDamage = 0.f;

	UAbilitySystemComponent& TargetASC = Data.Target;
	const float OldHealth = GetHealth();
	SetHealth(FMath::Clamp(OldHealth - Damage, 0.f, GetMaxHealth()));
	UE_LOG(LogMetaCombat, Verbose, TEXT("%s took %.2f (raw %.2f), HP %.1f -> %.1f"),
		*GetNameSafe(TargetASC.GetAvatarActor()), Damage, Raw, OldHealth, GetHealth());

	// 3.2 step 6: reflect raw × ratio back to the source, never re-reflecting.
	FGameplayTagContainer SpecTags;
	Data.EffectSpec.GetAllAssetTags(SpecTags);
	UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	if (Raw > 0.f && SourceASC && SourceASC != &TargetASC
		&& TargetASC.HasMatchingGameplayTag(MetaTags::Gimmick_Reflect)
		&& !SpecTags.HasTagExact(MetaTags::Damage_Type_Reflect))
	{
		FGameplayEffectSpecHandle Reflect = TargetASC.MakeOutgoingSpec(UGE_ReflectDamage::StaticClass(), 1.f, TargetASC.MakeEffectContext());
		Reflect.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, Raw * GetReflectRatio());
		TargetASC.ApplyGameplayEffectSpecToTarget(*Reflect.Data, SourceASC);
	}

	if (OldHealth > 0.f && GetHealth() <= 0.f)
	{
		FGameplayEventData Event;
		Event.EventTag = MetaTags::Event_Death;
		Event.Instigator = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
		Event.Target = TargetASC.GetAvatarActor();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetASC.GetAvatarActor(), MetaTags::Event_Death, Event);
	}
}
