#include "Combat/BossPatternComponent.h"
#include "NavigationSystem.h"
#include "Characters/MetaCharacterBase.h"
#include "Combat/MeleeHitboxComponent.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"

UBossPatternComponent::UBossPatternComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBossPatternComponent::SetPatternSet(const TArray<FBossPattern>& InPatterns)
{
	Patterns = InPatterns;
	NextReadyTime.Init(0.0, Patterns.Num());
	Phase = EPhase::Idle;
	Current = INDEX_NONE;
}

void UBossPatternComponent::Start(AActor* InTarget)
{
	Target = InTarget;
	bRunning = true;
	bPaused = false;
}

void UBossPatternComponent::Stop()
{
	bRunning = false;
	Phase = EPhase::Idle;
	Current = INDEX_NONE;
	NextReadyTime.Init(0.0, Patterns.Num());
	StopMoving();
}

int32 UBossPatternComponent::PickPattern(float Distance) const
{
	float TotalWeight = 0.f;
	TArray<int32, TInlineAllocator<8>> Eligible;
	for (int32 i = 0; i < Patterns.Num(); ++i)
	{
		const FBossPattern& P = Patterns[i];
		if (Distance >= P.MinRange && Distance <= P.MaxRange && LocalTime >= NextReadyTime[i] && P.Weight > 0.f)
		{
			Eligible.Add(i);
			TotalWeight += P.Weight;
		}
	}

	float Roll = FMath::FRand() * TotalWeight;
	for (int32 i : Eligible)
	{
		Roll -= Patterns[i].Weight;
		if (Roll <= 0.f)
		{
			return i;
		}
	}
	return Eligible.IsEmpty() ? INDEX_NONE : Eligible.Last();
}

void UBossPatternComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bRunning || bPaused || !Target.IsValid())
	{
		return;
	}

	LocalTime += DeltaTime;
	AActor* Owner = GetOwner();

	if (Phase == EPhase::Idle)
	{
		const float Distance = FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation());
		Current = PickPattern(Distance);
		if (Current == INDEX_NONE)
		{
			if (Distance <= AggroRange) { MoveTowardTarget(); } else { StopMoving(); }
			return;
		}
		StopMoving();
		BeginPhase(EPhase::Windup, Patterns[Current].Windup);
		if (AMetaCharacterBase* Character = Cast<AMetaCharacterBase>(Owner); Character && Patterns[Current].OptionalMontage)
		{
			const FBossPattern& P = Patterns[Current];
			Character->PlayAttackAnim(P.OptionalMontage, P.Windup + P.Active + P.Recovery);
		}
		OnPatternPhase.Broadcast(Patterns[Current].PatternId, true);
		return;
	}

	if (Phase == EPhase::Windup)
	{
		// Track the target during the telegraph, then commit.
		const FVector To = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		if (!To.IsNearlyZero())
		{
			Owner->SetActorRotation(To.Rotation());
		}
	}

	PhaseTimeLeft -= DeltaTime;
	if (PhaseTimeLeft > 0.f)
	{
		return;
	}

	const FBossPattern& P = Patterns[Current];
	switch (Phase)
	{
	case EPhase::Windup:
		OnPatternPhase.Broadcast(P.PatternId, false);
		ApplyHits();
		BeginPhase(EPhase::Active, P.Active);
		break;
	case EPhase::Active:
		BeginPhase(EPhase::Recovery, P.Recovery * RecoveryScale);
		break;
	case EPhase::Recovery:
		NextReadyTime[Current] = LocalTime + P.Cooldown;
		Current = INDEX_NONE;
		Phase = EPhase::Idle;
		break;
	default:
		break;
	}
}

void UBossPatternComponent::BeginPhase(EPhase NewPhase, float Duration)
{
	Phase = NewPhase;
	PhaseTimeLeft = Duration;
}

void UBossPatternComponent::ApplyHits()
{
	// shortcut: one overlap at the start of the active window; upgrade to per-tick sweeps for long active windows.
	AActor* Owner = GetOwner();
	const UMeleeHitboxComponent* Hitbox = Owner->FindComponentByClass<UMeleeHitboxComponent>();
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!Hitbox || !SourceASC)
	{
		return;
	}

	const FBossPattern& P = Patterns[Current];
	for (AActor* Hit : Hitbox->QueryHits(P.HitboxExtent, P.HitboxOffset))
	{
		if (Hit != Target.Get())
		{
			continue; // bosses only hurt the player, never each other
		}
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Hit))
		{
			const TSubclassOf<UGameplayEffect> GE = P.bInstantKill ? TSubclassOf<UGameplayEffect>(UGE_InstantKill::StaticClass()) : TSubclassOf<UGameplayEffect>(UGE_Damage::StaticClass());
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(GE, 1.f, SourceASC->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, P.FixedDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		}
	}
}

void UBossPatternComponent::MoveTowardTarget()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}
	// With no navmesh MoveToActor "succeeds" but never moves; only path-follow when nav data exists.
	UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	AAIController* AI = Cast<AAIController>(Pawn->GetController());
	if (AI && Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
	{
		if (AI->GetMoveStatus() == EPathFollowingStatus::Moving || AI->MoveToActor(Target.Get(), ApproachAcceptance) != EPathFollowingRequestResult::Failed)
		{
			return;
		}
	}
	// No navmesh: slide straight at the target (fits the T-pose defense entities).
	const FVector To = Target->GetActorLocation() - Pawn->GetActorLocation();
	if (To.Size2D() > ApproachAcceptance)
	{
		Pawn->AddMovementInput(To.GetSafeNormal2D());
	}
}

void UBossPatternComponent::StopMoving()
{
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (AAIController* AI = Cast<AAIController>(Pawn->GetController()))
		{
			AI->StopMovement();
		}
	}
}
