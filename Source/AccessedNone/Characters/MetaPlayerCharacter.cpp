#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayAbility.h"
#include "Combat/MetaGameplayEffects.h"
#include "Combat/MeleeHitboxComponent.h"
#include "Core/AccessedNoneSettings.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/EncounterSubsystem.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Characters/MetaProjectile.h"
#include "Core/MetaGameMode.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaInteractable.h"
#include "Particles/ParticleSystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMetaPlayerCharacter::AMetaPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	InitEffect = UGE_Init_Knight::StaticClass();
	DefaultAbilities = { UGA_LightAttack::StaticClass(), UGA_HeavyAttack::StaticClass(), UGA_Dodge::StaticClass(), UGA_Heal::StaticClass() };

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationPitch = bUseControllerRotationYaw = bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->JumpZVelocity = 600.f;
	Move->AirControl = 0.3f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = UAccessedNoneSettings::Get().CameraArmLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Hitbox = CreateDefaultSubobject<UMeleeHitboxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(RootComponent);

	AimMarker = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("AimMarker"));
	AimMarker->SetupAttachment(RootComponent);
	AimMarker->SetUsingAbsoluteLocation(true);
	AimMarker->SetUsingAbsoluteRotation(true);
	AimMarker->bAutoActivate = false;

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -97.f), FRotator(0.f, -90.f, 0.f));
	// E5 letterbox silhouette (spec 8.9).
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(1);
}

void AMetaPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	for (const TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(Ability, 1));
	}
	RefillHealFlasks();
	UpdateMoveSpeed();
	SetHero(0);
	DamageHandle = FMetaCombatEvents::OnDamage.AddUObject(this, &AMetaPlayerCharacter::HandleAnyDamage);
}

void AMetaPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	FMetaCombatEvents::OnDamage.Remove(DamageHandle);
	Super::EndPlay(Reason);
}

IMetaInteractable* AMetaPlayerCharacter::FindInteractable() const
{
	constexpr float InteractRange = 250.f;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Interact), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects),
		FCollisionShape::MakeSphere(InteractRange), Params);
	IMetaInteractable* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		IMetaInteractable* I = Cast<IMetaInteractable>(O.GetActor());
		const float DistSq = I ? FVector::DistSquared(O.GetActor()->GetActorLocation(), GetActorLocation()) : 0.f;
		if (I && DistSq < BestDistSq && !I->GetInteractPrompt().IsEmpty())
		{
			Best = I;
			BestDistSq = DistSq;
		}
	}
	return Best;
}

void AMetaPlayerCharacter::Interact()
{
	if (IMetaInteractable* I = IsDead() ? nullptr : FindInteractable())
	{
		I->Interact(this);
	}
}

void AMetaPlayerCharacter::Revive(const FTransform& At)
{
	AbilitySystem->SetLooseGameplayTagCount(MetaTags::State_Dead, 0);
	AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
	AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetStaminaAttribute(), Attributes->GetMaxStamina());
	RefillHealFlasks();
	SetActorLocationAndRotation(At.GetLocation(), At.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetHero(HeroIndex); // back from the held death pose to the hero's anim BP
}

bool AMetaPlayerCharacter::IsDodgeOnCooldown() const
{
	return GetWorld()->GetTimeSeconds() - LastDodgeTime < UAccessedNoneSettings::Get().DodgeCooldown;
}

void AMetaPlayerCharacter::HandleAnyDamage(AActor* Target, float Damage, bool bImmune)
{
	// Flinch on a landed enemy hit; not while rolling or dead (the lethal hit plays the death instead).
	const FHeroDef* H = GetHeroDef();
	if (Target != this || bImmune || Damage <= 0.f || IsDead() || GetHealth() <= 0.f || RollElapsed >= 0.f || !H)
	{
		return;
	}
	if (UAnimSequenceBase* Flinch = H->HitReactAnim.LoadSynchronous())
	{
		PlayAttackAnim(Flinch, Flinch->GetPlayLength());
	}
}

void AMetaPlayerCharacter::SetHero(int32 Index)
{
	const TArray<FHeroDef>& Heroes = UAccessedNoneSettings::Get().Heroes;
	if (!Heroes.IsValidIndex(Index))
	{
		return;
	}
	const FHeroDef& H = Heroes[Index];
	HeroIndex = Index;
	ComboStep = 0;
	AttackSlot = TEXT("DefaultSlot");
	USkeletalMeshComponent* M = GetMesh();
	// Paragon anim BPs write per-slot material overrides (intro dissolve); drop them or the next hero renders with the previous hero's materials.
	M->EmptyOverrideMaterials();
	M->SetSkeletalMeshAsset(H.Mesh.LoadSynchronous());
	if (UClass* Anim = H.AnimClass.LoadSynchronous())
	{
		M->SetAnimInstanceClass(Anim);
	}
	M->SetRelativeScale3D(FVector(H.MeshScale));
	// Cloth keeps the previous hero's sim state after a mesh swap and explodes; rebuild it and teleport-reset.
	M->RecreateClothingActors();
	M->ForceClothNextUpdateTeleportAndReset();
	SuppressIntroMontages();
	if (!H.LightAttackAnims.IsEmpty())
	{
		if (UAnimMontage* First = Cast<UAnimMontage>(H.LightAttackAnims[0].LoadSynchronous()); First && !First->SlotAnimTracks.IsEmpty())
		{
			AttackSlot = First->SlotAnimTracks[0].SlotName;
		}
	}
	AttackExtent = H.AttackExtent;
	AttackOffset = H.AttackOffset;
	// Over-the-shoulder view for the archer so the crosshair and target marker aren't hidden by her back.
	CameraBoom->SocketOffset = H.bRanged ? FVector(0.f, 70.f, 45.f) : FVector::ZeroVector;
}

void AMetaPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickStamina(DeltaSeconds);
	TickLockOn(DeltaSeconds);
	TickRoll(DeltaSeconds);
	TickAim();
	TickCameraDistance(DeltaSeconds);
}

float AMetaPlayerCharacter::GetCameraArmLength() const
{
	return CameraBoom->TargetArmLength;
}

void AMetaPlayerCharacter::TickCameraDistance(float DeltaSeconds)
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	float Desired = S.CameraArmLength;
	if (const ACharacter* Target = Cast<ACharacter>(LockOnTarget.Get()))
	{
		constexpr float NormalHalfHeight = 96.f;
		const float HalfHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Desired = FMath::Min(S.CameraArmMax, S.CameraArmLength + S.LockOnArmBonus + FMath::Max(0.f, HalfHeight - NormalHalfHeight) * S.LockOnArmPerHeight);
	}
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Desired, DeltaSeconds, S.CameraArmInterpSpeed);
}

const FHeroDef* AMetaPlayerCharacter::GetHeroDef() const
{
	const TArray<FHeroDef>& Heroes = UAccessedNoneSettings::Get().Heroes;
	return Heroes.IsValidIndex(HeroIndex) ? &Heroes[HeroIndex] : nullptr;
}

bool AMetaPlayerCharacter::IsRanged() const
{
	const FHeroDef* H = GetHeroDef();
	return H && H->bRanged;
}

FVector AMetaPlayerCharacter::GetAimPoint(AActor*& OutTarget, bool& bOutHit) const
{
	OutTarget = nullptr;
	bOutHit = false;
	if (LockOnTarget.IsValid())
	{
		OutTarget = LockOnTarget.Get();
		bOutHit = true;
		return OutTarget->GetActorLocation();
	}
	const float Range = GetHeroDef() ? GetHeroDef()->AimRange : 4000.f;
	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + FollowCamera->GetForwardVector() * (Range + CameraBoom->TargetArmLength);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Aim), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) || GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_Pawn), Params))
	{
		bOutHit = true;
		if (const AMetaCharacterBase* C = Cast<AMetaCharacterBase>(Hit.GetActor()); C && !C->IsDead() && !C->IsHidden())
		{
			OutTarget = Hit.GetActor();
		}
		return Hit.ImpactPoint;
	}
	return End;
}

void AMetaPlayerCharacter::SetAttackFacing(bool bFacing)
{
	bAttackFacing = bFacing;
	if (!IsLockedOn())
	{
		GetCharacterMovement()->bOrientRotationToMovement = !bFacing;
	}
}

void AMetaPlayerCharacter::TickAim()
{
	if (bAttackFacing && !IsLockedOn())
	{
		AActor* FaceTarget = nullptr;
		bool bFaceHit = false;
		const FVector To = (GetAimPoint(FaceTarget, bFaceHit) - GetActorLocation()).GetSafeNormal2D();
		if (!To.IsNearlyZero())
		{
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), To.Rotation(), GetWorld()->GetDeltaSeconds(), 25.f));
		}
	}

	const FHeroDef* H = GetHeroDef();
	const bool bShow = H && H->bRanged && !IsDead();
	if (!bShow)
	{
		if (AimMarker->IsActive()) { AimMarker->Deactivate(); }
		return;
	}
	if (!AimMarker->IsActive())
	{
		AimMarker->SetTemplate(H->AimMarkerFX.LoadSynchronous());
		AimMarker->Activate(true);
	}
	AActor* Target = nullptr;
	bool bHit = false;
	const FVector Point = GetAimPoint(Target, bHit);
	AimMarker->SetVisibility(bHit);
	// On an enemy the marker sits at its feet so it reads as "this one gets hit".
	AimMarker->SetWorldLocation(Target ? Target->GetActorLocation() - FVector(0, 0, 90.f) : Point);
}

void AMetaPlayerCharacter::FireProjectile(float MotionValue, FGameplayTag DamageType, bool bHeavy)
{
	const FHeroDef* H = GetHeroDef();
	if (!H)
	{
		return;
	}
	AActor* Target = nullptr;
	bool bHit = false;
	FVector Aim = GetAimPoint(Target, bHit);
	if (Target)
	{
		Aim = Target->GetActorLocation(); // the trace may touch an outstretched limb outside the body capsule (big bosses)
	}
	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 60.f + FVector(0, 0, 40.f);
	const FVector Dir = (Aim - Muzzle).GetSafeNormal();
	if (!IsLockedOn())
	{
		SetActorRotation(FRotator(0.f, Dir.Rotation().Yaw, 0.f)); // turn to the shot
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AMetaProjectile* Arrow = GetWorld()->SpawnActor<AMetaProjectile>(AMetaProjectile::StaticClass(), Muzzle, Dir.Rotation(), SpawnParams))
	{
		Arrow->Launch(AbilitySystem, Dir, H->ProjectileSpeed, MotionValue, DamageType,
			(bHeavy ? H->HeavyProjectileFX : H->ProjectileFX).LoadSynchronous(), H->HitFX.LoadSynchronous());
	}
}

void AMetaPlayerCharacter::StartRollVisual(const FVector& WorldDir, float Seconds)
{
	if (RollElapsed < 0.f)
	{
		MeshBaseTransform = GetMesh()->GetRelativeTransform();
	}
	// Rotating "up" about (Up x Dir) tips the head toward Dir: a forward roll in that direction.
	RollAxisLocal = GetActorTransform().InverseTransformVectorNoScale(FVector::CrossProduct(FVector::UpVector, WorldDir.GetSafeNormal2D())).GetSafeNormal();
	RollSeconds = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	RollElapsed = 0.f;
}

void AMetaPlayerCharacter::TickRoll(float DeltaSeconds)
{
	if (RollElapsed < 0.f)
	{
		return;
	}
	RollElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(RollElapsed / RollSeconds, 0.f, 1.f);
	const float Angle = UE_TWO_PI * FMath::InterpEaseInOut(0.f, 1.f, Alpha, 1.6f);
	const FQuat Spin(RollAxisLocal, Angle);
	// Pivot = capsule center (the mesh origin sits at the feet). Crouch the body slightly while rolling.
	const FVector Tucked = MeshBaseTransform.GetLocation() * FMath::Lerp(1.f, 0.55f, FMath::Sin(Alpha * PI));
	GetMesh()->SetRelativeLocationAndRotation(Spin.RotateVector(Tucked), Spin * MeshBaseTransform.GetRotation());
	if (Alpha >= 1.f)
	{
		GetMesh()->SetRelativeTransform(MeshBaseTransform);
		RollElapsed = -1.f;
	}
}

// --- Input -----------------------------------------------------------------

void AMetaPlayerCharacter::Move(const FVector2D& Input)
{
	if (!Controller)
	{
		return;
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Input.X);
}

void AMetaPlayerCharacter::Look(const FVector2D& Input)
{
	if (IsLockedOn())
	{
		return; // camera is driven by the lock-on target
	}
	const UMetaSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>();
	const float Sensitivity = Settings->GetValue(EMetaSettingKey::MouseSensitivity);
	const float InvertY = Settings->GetValue(EMetaSettingKey::InvertY) > 0.5f ? -1.f : 1.f;
	AddControllerYawInput(Input.X * Sensitivity);
	AddControllerPitchInput(Input.Y * Sensitivity * InvertY);
}

void AMetaPlayerCharacter::TryJump()
{
	if (CanJump() && TrySpendStamina(UAccessedNoneSettings::Get().StaminaCostJump))
	{
		Jump();
	}
}

void AMetaPlayerCharacter::SetDashing(bool bWantsDash)
{
	if (bWantsDash && Attributes->GetStamina() <= 0.f)
	{
		bWantsDash = false;
	}
	if (bDashing != bWantsDash)
	{
		bDashing = bWantsDash;
		UpdateMoveSpeed();
	}
}

bool AMetaPlayerCharacter::ActivateAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	return !IsDead() && AbilitySystem->TryActivateAbilityByClass(AbilityClass);
}

void AMetaPlayerCharacter::PlayAttackMotion(bool bHeavy, float TotalSeconds)
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	if (!S.Heroes.IsValidIndex(HeroIndex))
	{
		return;
	}
	const FHeroDef& H = S.Heroes[HeroIndex];
	if (bHeavy)
	{
		PlayAttackAnim(H.HeavyAttackAnim.LoadSynchronous(), TotalSeconds);
		return;
	}
	if (H.LightAttackAnims.IsEmpty())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	ComboStep = Now - LastLightAttackTime <= S.ComboResetSeconds ? (ComboStep + 1) % H.LightAttackAnims.Num() : 0;
	LastLightAttackTime = Now;
	PlayAttackAnim(H.LightAttackAnims[ComboStep].LoadSynchronous(), TotalSeconds);
}

// --- Stamina / heal ---------------------------------------------------------

bool AMetaPlayerCharacter::TrySpendStamina(float Cost)
{
	if (Attributes->GetStamina() <= 0.f)
	{
		return false;
	}
	AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetStaminaAttribute(), Attributes->GetStamina() - Cost);
	LastStaminaSpendTime = GetWorld()->GetTimeSeconds();
	return true;
}

void AMetaPlayerCharacter::TickStamina(float DeltaSeconds)
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	const bool bMoving = GetVelocity().SizeSquared2D() > 1.f;
	if (bDashing && bMoving)
	{
		AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetStaminaAttribute(), Attributes->GetStamina() - S.StaminaCostDashPerSecond * DeltaSeconds);
		LastStaminaSpendTime = GetWorld()->GetTimeSeconds();
		if (Attributes->GetStamina() <= 0.f)
		{
			SetDashing(false);
		}
		return;
	}

	if (GetWorld()->GetTimeSeconds() - LastStaminaSpendTime >= S.StaminaRegenDelay && Attributes->GetStamina() < Attributes->GetMaxStamina())
	{
		AbilitySystem->SetNumericAttributeBase(UMetaCombatAttributeSet::GetStaminaAttribute(), Attributes->GetStamina() + S.StaminaRegenPerSecond * DeltaSeconds);
	}
}

bool AMetaPlayerCharacter::ConsumeHealFlask()
{
	if (HealFlaskCount <= 0)
	{
		return false;
	}
	--HealFlaskCount;
	return true;
}

void AMetaPlayerCharacter::RefillHealFlasks()
{
	HealFlaskCount = UAccessedNoneSettings::Get().HealFlaskMax;
}

void AMetaPlayerCharacter::SetHealing(bool bInHealing)
{
	bHealing = bInHealing;
	UpdateMoveSpeed();
}

void AMetaPlayerCharacter::UpdateMoveSpeed()
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	float Speed = bDashing ? S.DashSpeed : S.WalkSpeed;
	if (bHealing)
	{
		Speed *= S.HealMoveSpeedScale;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

// --- Lock-on ----------------------------------------------------------------

void AMetaPlayerCharacter::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ClearLockOn();
		return;
	}
	if (AActor* Target = FindLockOnTarget())
	{
		LockOnTarget = Target;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
}

void AMetaPlayerCharacter::ClearLockOn()
{
	LockOnTarget.Reset();
	GetCharacterMovement()->bOrientRotationToMovement = !bAttackFacing;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

AActor* AMetaPlayerCharacter::FindLockOnTarget() const
{
	const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
	const FVector ViewDir = FollowCamera->GetForwardVector().GetSafeNormal2D();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(S.LockOnConeAngle * 0.5f));

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockOn), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(S.LockOnRadius), Params);

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& O : Overlaps)
	{
		const AMetaCharacterBase* Candidate = Cast<AMetaCharacterBase>(O.GetActor());
		if (!Candidate || Candidate->IsDead() || Candidate->IsHidden() || Candidate->IsA<AMetaPlayerCharacter>())
		{
			continue;
		}
		const FVector To = Candidate->GetActorLocation() - GetActorLocation();
		if (FVector::DotProduct(To.GetSafeNormal2D(), ViewDir) < MinDot)
		{
			continue;
		}
		if (To.SizeSquared() < BestDistSq)
		{
			BestDistSq = To.SizeSquared();
			Best = const_cast<AMetaCharacterBase*>(Candidate);
		}
	}
	return Best;
}

void AMetaPlayerCharacter::TickLockOn(float DeltaSeconds)
{
	if (!LockOnTarget.IsValid())
	{
		if (!GetCharacterMovement()->bOrientRotationToMovement)
		{
			ClearLockOn(); // target destroyed
		}
		return;
	}

	const AMetaCharacterBase* Target = Cast<AMetaCharacterBase>(LockOnTarget.Get());
	const float Range = UAccessedNoneSettings::Get().LockOnRadius;
	if ((Target && Target->IsDead()) || FVector::DistSquared(LockOnTarget->GetActorLocation(), GetActorLocation()) > FMath::Square(Range))
	{
		ClearLockOn();
		return;
	}

	if (Controller)
	{
		const FRotator Desired = (LockOnTarget->GetActorLocation() - FollowCamera->GetComponentLocation()).Rotation();
		Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), Desired, DeltaSeconds, 10.f));
	}
}

void AMetaPlayerCharacter::HandleDeath(const FGameplayEventData* Payload)
{
	ClearLockOn();
	SetDashing(false);
	const bool bWasDead = IsDead();
	Super::HandleDeath(Payload);
	// Collapse and hold the last frame. shortcut: single-node anim mode stays until SetHero; restore it when breakpoint respawn (S3-T01) revives in place.
	if (const FHeroDef* H = GetHeroDef(); H && !bWasDead)
	{
		if (UAnimSequenceBase* Death = H->DeathAnim.LoadSynchronous())
		{
			GetMesh()->PlayAnimation(Death, false);
		}
	}
	if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>())
	{
		Encounters->NotifyPlayerDied();
	}
	if (AMetaGameMode* GM = GetWorld()->GetAuthGameMode<AMetaGameMode>())
	{
		GM->HandlePlayerDeath(this);
	}
}
