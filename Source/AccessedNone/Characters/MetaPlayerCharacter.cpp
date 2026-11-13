#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayAbility.h"
#include "Combat/MetaGameplayEffects.h"
#include "Combat/MeleeHitboxComponent.h"
#include "Core/AccessedNoneSettings.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/EncounterSubsystem.h"
#include "AbilitySystemComponent.h"
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
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Hitbox = CreateDefaultSubobject<UMeleeHitboxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(RootComponent);

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
}

void AMetaPlayerCharacter::SetHero(int32 Index)
{
	const TArray<FHeroDef>& Heroes = UAccessedNoneSettings::Get().Heroes;
	if (!Heroes.IsValidIndex(Index))
	{
		return;
	}
	const FHeroDef& H = Heroes[Index];
	USkeletalMeshComponent* M = GetMesh();
	M->SetSkeletalMeshAsset(H.Mesh.LoadSynchronous());
	if (UClass* Anim = H.AnimClass.LoadSynchronous())
	{
		M->SetAnimInstanceClass(Anim);
	}
	M->SetRelativeScale3D(FVector(H.MeshScale));
	AttackExtent = H.AttackExtent;
	AttackOffset = H.AttackOffset;
}

void AMetaPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickStamina(DeltaSeconds);
	TickLockOn(DeltaSeconds);
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
	GetCharacterMovement()->bOrientRotationToMovement = true;
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
		if (!Candidate || Candidate->IsDead() || Candidate->IsA<AMetaPlayerCharacter>())
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
	Super::HandleDeath(Payload);
	if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>())
	{
		Encounters->NotifyPlayerDied();
	}
}
