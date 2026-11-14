#include "Characters/AssassinBoss.h"
#include "Combat/BossPatternComponent.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaGameplayTags.h"
#include "Gimmick/GimmickCondition.h"
#include "AbilitySystemComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"

namespace
{
	constexpr float GlintSeconds = 0.18f;
	constexpr float GlintIntensity = 120.f;
	constexpr float GlintIntervalCalm = 4.f;  // tier 0
	constexpr float GlintIntervalUneasy = 1.6f; // tier >= 1: "이상 징후 강화" (spec 6.3)
}

AAssassinBoss::AAssassinBoss()
{
	PrimaryActorTick.bCanEverTick = true;
	UGimmickCondition_SettingThreshold* Gamma = CreateDefaultSubobject<UGimmickCondition_SettingThreshold>(TEXT("GammaCondition"));
	Gamma->Key = EMetaSettingKey::Gamma;
	Gamma->Threshold = 0.95f;
	Gamma->bGreaterOrEqual = true;
	Gimmick->Conditions.Add(Gamma);
	Gimmick->SettingsToResetOnStart.Add(EMetaSettingKey::Gamma, 0.2f);
	Gimmick->BreakingDuration = 1.f; // covers the 0.8 s whiteout (spec 12.3)

	EyeGlint = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeGlint"));
	EyeGlint->SetupAttachment(GetMesh(), TEXT("head"));
	EyeGlint->SetLightColor(FLinearColor(1.f, 0.15f, 0.1f));
	EyeGlint->SetAttenuationRadius(140.f);
	EyeGlint->SetIntensity(0.f);
	EyeGlint->SetCastShadows(false);
}

void AAssassinBoss::BeginPlay()
{
	Super::BeginPlay();
	SetStealthed(true);
}

void AAssassinBoss::SetStealthed(bool bInStealthed)
{
	bStealthed = bInStealthed;
	GetMesh()->SetVisibility(!bInStealthed, false);
	if (bInStealthed)
	{
		AbilitySystem->AddLooseGameplayTag(MetaTags::Gimmick_Stealth);
	}
	else
	{
		AbilitySystem->SetLooseGameplayTagCount(MetaTags::Gimmick_Stealth, 0);
		EyeGlint->SetIntensity(0.f);
	}
}

void AAssassinBoss::HandleGimmickStateChanged(EGimmickState Old, EGimmickState New)
{
	Super::HandleGimmickStateChanged(Old, New);
	switch (New)
	{
	case EGimmickState::Dormant:
	case EGimmickState::Armored:
		GetWorldTimerManager().ClearTimer(StunTimer);
		SetStealthed(true);
		break;
	case EGimmickState::Breaking:
		SetStealthed(false); // the whiteout exposes it
		break;
	case EGimmickState::Vulnerable:
	{
		// Dazzled: 6 s stun, no attacks.
		const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UGE_Stun::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_Duration, StunSeconds);
		AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		Patterns->Pause();
		GetWorldTimerManager().SetTimer(StunTimer, this, &AAssassinBoss::EndStun, StunSeconds, false);
		break;
	}
	case EGimmickState::Defeated:
		GetWorldTimerManager().ClearTimer(StunTimer);
		break;
	}
}

void AAssassinBoss::EndStun()
{
	if (Gimmick->GetState() == EGimmickState::Vulnerable && !IsDummyOrDead())
	{
		Patterns->Resume();
	}
}

void AAssassinBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bStealthed)
	{
		return;
	}
	const float T = GetWorld()->GetTimeSeconds();
	if (T >= NextGlint)
	{
		GlintEnd = T + GlintSeconds;
		NextGlint = T + (HintTier >= 1 ? GlintIntervalUneasy : GlintIntervalCalm) * FMath::FRandRange(0.7f, 1.3f);
	}
	EyeGlint->SetIntensity(T < GlintEnd ? GlintIntensity : 0.f);
}
