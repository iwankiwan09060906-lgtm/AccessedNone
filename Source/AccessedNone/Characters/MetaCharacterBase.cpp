#include "Characters/MetaCharacterBase.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaLog.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AMetaCharacterBase::AMetaCharacterBase()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	// Default subobject of the actor → the ASC discovers it automatically.
	Attributes = CreateDefaultSubobject<UMetaCombatAttributeSet>(TEXT("Attributes"));
	// Fixed camera distance: characters never pull the spring arm in (only level geometry does).
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AMetaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (InitEffect)
	{
		AbilitySystem->ApplyGameplayEffectToSelf(InitEffect->GetDefaultObject<UGameplayEffect>(), 1.f, AbilitySystem->MakeEffectContext());
	}
	AbilitySystem->GenericGameplayEventCallbacks.FindOrAdd(MetaTags::Event_Death).AddUObject(this, &AMetaCharacterBase::HandleDeath);
	SuppressIntroMontages();
}

void AMetaCharacterBase::SuppressIntroMontages()
{
	// The anim BP starts its intro montage on its first update, so keep stopping montages for a short window.
	constexpr double IntroWindowSeconds = 1.0;
	IntroMontageWindowEnd = GetWorld()->GetTimeSeconds() + IntroWindowSeconds;
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		Anim->StopAllMontages(0.f);
		Anim->OnMontageStarted.AddUniqueDynamic(this, &AMetaCharacterBase::HandleMontageStarted);
	}
	// The intro's dissolve writes per-slot material overrides (fully dissolved = invisible); drop them after the window.
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { GetMesh()->EmptyOverrideMaterials(); }), static_cast<float>(IntroWindowSeconds) + 0.1f, false);
}

void AMetaCharacterBase::HandleMontageStarted(UAnimMontage* Montage)
{
	if (GetWorld()->GetTimeSeconds() < IntroMontageWindowEnd)
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.f, Montage);
		GetMesh()->EmptyOverrideMaterials();
	}
}

float AMetaCharacterBase::GetHealth() const { return Attributes->GetHealth(); }
float AMetaCharacterBase::GetMaxHealth() const { return Attributes->GetMaxHealth(); }
bool AMetaCharacterBase::IsDead() const { return AbilitySystem->HasMatchingGameplayTag(MetaTags::State_Dead); }

void AMetaCharacterBase::HandleDeath(const FGameplayEventData* /*Payload*/)
{
	if (IsDead())
	{
		return;
	}
	UE_LOG(LogMetaCombat, Log, TEXT("%s died"), *GetName());
	AbilitySystem->AddLooseGameplayTag(MetaTags::State_Dead);
	AbilitySystem->CancelAllAbilities();
	GetCharacterMovement()->DisableMovement();
	OnDeath();
}

void AMetaCharacterBase::PlayAttackAnim(UAnimSequenceBase* Anim, float TotalSeconds)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!Anim || !AnimInstance || TotalSeconds <= 0.f)
	{
		return;
	}
	IntroMontageWindowEnd = 0.0; // a real attack must not be swallowed by the intro suppression
	const float Rate = Anim->GetPlayLength() / TotalSeconds;
	if (UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
	{
		if (!Montage->SlotAnimTracks.IsEmpty())
		{
			AttackSlot = Montage->SlotAnimTracks[0].SlotName;
		}
		AnimInstance->Montage_Play(Montage, Rate);
	}
	else
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, AttackSlot, 0.1f, 0.2f, Rate);
	}
}
