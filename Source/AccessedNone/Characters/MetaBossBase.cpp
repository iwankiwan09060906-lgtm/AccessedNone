#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Combat/BossPatternComponent.h"
#include "Combat/MeleeHitboxComponent.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaLog.h"
#include "Meta/EncounterSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "TimerManager.h"

AMetaBossBase::AMetaBossBase()
{
	Gimmick = CreateDefaultSubobject<UGimmickComponent>(TEXT("Gimmick"));
	Patterns = CreateDefaultSubobject<UBossPatternComponent>(TEXT("Patterns"));
	Hitbox = CreateDefaultSubobject<UMeleeHitboxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(RootComponent);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
}

void AMetaBossBase::BeginPlay()
{
	if (BossData && !BossData->Mesh.IsNull())
	{
		GetMesh()->SetSkeletalMeshAsset(BossData->Mesh.LoadSynchronous());
		GetMesh()->SetAnimInstanceClass(BossData->AnimClass.LoadSynchronous());
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
		GetMesh()->SetRelativeScale3D(FVector(BossData->MeshScale));
	}
	if (BossData && !FMath::IsNearlyEqual(BossData->BodyScale, 1.f))
	{
		// Grow upward so the bigger capsule doesn't start inside the floor.
		const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() * GetActorScale3D().Z;
		SetActorScale3D(FVector(BossData->BodyScale));
		AddActorWorldOffset(FVector(0.f, 0.f, HalfHeight * (BossData->BodyScale - 1.f)));
	}
	Super::BeginPlay(); // after the mesh swap: base BeginPlay suppresses the new anim instance's intro montage
	SpawnTransform = GetActorTransform();
	Gimmick->OnStateChanged.AddDynamic(this, &AMetaBossBase::HandleGimmickStateChanged);
}

void AMetaBossBase::StartEncounter(AActor* PlayerTarget)
{
	EncounterTarget = PlayerTarget;
	Gimmick->BeginEncounter();
}

void AMetaBossBase::ResetEncounter()
{
	Gimmick->ResetEncounter();
}

void AMetaBossBase::HandleGimmickStateChanged(EGimmickState Old, EGimmickState New)
{
	switch (New)
	{
	case EGimmickState::Armored:
	{
		if (Old == EGimmickState::Dormant)
		{
			// HP from the player's AttackPower at encounter start (spec 3.1).
			const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
			float Multiplier = S.BossHPMultiplier;
			if (BossData && BossData->HPMultiplierOverride > 0.f) { Multiplier = BossData->HPMultiplierOverride; }
			else if (BossData && BossData->bIsMinion) { Multiplier = S.MinionHPMultiplier; }

			const UAbilitySystemComponent* PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(EncounterTarget.Get());
			const float D = PlayerASC ? PlayerASC->GetNumericAttribute(UMetaCombatAttributeSet::GetAttackPowerAttribute()) : S.KnightAttackPower;
			AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetMaxHealthAttribute(), D * Multiplier);
			AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), D * Multiplier);
		}
		else
		{
			// Non-latched gimmick re-armed.
			AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(UGE_Boss_Vulnerable::StaticClass(), nullptr);
		}
		if (BossData)
		{
			for (const TSubclassOf<UGameplayEffect>& GE : BossData->ArmoredEffects)
			{
				AbilitySystem->ApplyGameplayEffectToSelf(GE->GetDefaultObject<UGameplayEffect>(), 1.f, AbilitySystem->MakeEffectContext());
			}
		}
		Patterns->SetPatternSet(BossData ? BossData->PatternSet_Armored : TArray<FBossPattern>());
		if (!IsDummy())
		{
			Patterns->Start(EncounterTarget.Get());
		}
		break;
	}
	case EGimmickState::Breaking:
		Patterns->Pause();
		AbilitySystem->AddLooseGameplayTag(MetaTags::State_Breaking);
		OnBreakingStarted();
		break;

	case EGimmickState::Vulnerable:
		if (BossData)
		{
			for (const TSubclassOf<UGameplayEffect>& GE : BossData->ArmoredEffects)
			{
				AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(GE, nullptr);
			}
		}
		AbilitySystem->ApplyGameplayEffectToSelf(UGE_Boss_Vulnerable::StaticClass()->GetDefaultObject<UGameplayEffect>(), 1.f, AbilitySystem->MakeEffectContext());
		AbilitySystem->RemoveLooseGameplayTag(MetaTags::State_Breaking);
		Patterns->SetPatternSet(BossData ? BossData->PatternSet_Vulnerable : TArray<FBossPattern>());
		if (!IsDummy())
		{
			Patterns->Resume();
		}
		{
			FGameplayEventData Event;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, MetaTags::Event_Gimmick_Broken, Event);
		}
		OnVulnerable();
		break;

	case EGimmickState::Defeated:
		Patterns->Stop();
		RemoveStateEffects();
		PlayDeathAndVanish();
		OnDefeated();
		if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>())
		{
			Encounters->NotifyBossDefeated(this);
		}
		break;

	case EGimmickState::Dormant:
		GetWorldTimerManager().ClearTimer(DeathTimer);
		SetPresent(true); // a reset brings defeated bosses back
		Patterns->Stop();
		RemoveStateEffects();
		AbilitySystem->SetLooseGameplayTagCount(MetaTags::State_Dead, 0);
		AbilitySystem->SetLooseGameplayTagCount(MetaTags::State_Breaking, 0);
		SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::ResetPhysics);
		GetCharacterMovement()->SetMovementMode(GetCharacterMovement()->DefaultLandMovementMode);
		AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
		break;
	}
}

void AMetaBossBase::RemoveStateEffects()
{
	if (BossData)
	{
		for (const TSubclassOf<UGameplayEffect>& GE : BossData->ArmoredEffects)
		{
			AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(GE, nullptr);
		}
	}
	AbilitySystem->RemoveActiveGameplayEffectBySourceEffect(UGE_Boss_Vulnerable::StaticClass(), nullptr);
}

void AMetaBossBase::HandleDeath(const FGameplayEventData* Payload)
{
	Super::HandleDeath(Payload);
	Gimmick->NotifyDefeated();
}

bool AMetaBossBase::IsDummy() const
{
	return BossData && BossData->bTrainingDummy;
}

void AMetaBossBase::PlayDeathAndVanish()
{
	float AnimSeconds = 0.f;
	if (UAnimSequenceBase* Death = BossData ? BossData->DeathAnim.LoadSynchronous() : nullptr)
	{
		// Single-node playback without looping holds the final (collapsed) frame.
		GetMesh()->PlayAnimation(Death, false);
		AnimSeconds = Death->GetPlayLength();
	}
	const float Delay = AnimSeconds + (BossData ? BossData->VanishDelay : 2.f);
	GetWorldTimerManager().SetTimer(DeathTimer, this, &AMetaBossBase::Vanish, FMath::Max(Delay, 0.01f), false);
}

void AMetaBossBase::Vanish()
{
	SetPresent(false);
	if (IsDummy())
	{
		GetWorldTimerManager().SetTimer(DeathTimer, this, &AMetaBossBase::Revive, FMath::Max(BossData->RespawnDelay, 0.01f), false);
	}
}

void AMetaBossBase::Revive()
{
	Gimmick->ResetEncounter(); // Dormant: back at spawn, full HP, visible
	if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>(); Encounters && !EncounterId.IsNone())
	{
		Encounters->RestartEncounter(EncounterId, this);
	}
}

void AMetaBossBase::SetPresent(bool bPresent)
{
	SetActorHiddenInGame(!bPresent);
	SetActorEnableCollision(bPresent);
	if (bPresent && GetMesh()->GetAnimationMode() != EAnimationMode::AnimationBlueprint)
	{
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint); // leave the held death pose
		SuppressIntroMontages();
	}
}
