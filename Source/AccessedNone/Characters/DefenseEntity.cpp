#include "Characters/DefenseEntity.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Swing poses: yaw/pitch snaps of the whole mesh, no in-betweens.
	const FRotator SwingPoses[] = { FRotator(0.f, -70.f, 0.f), FRotator(-25.f, 60.f, 0.f), FRotator(0.f, 20.f, 0.f) };
}

ADefenseEntity::ADefenseEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true; // fall without a controller
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (Manny.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(Manny.Object);
	}
	GetMesh()->SetAnimInstanceClass(nullptr); // reference pose = T-pose
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	GridMaterial = FSoftObjectPath(TEXT("/Game/Core/Materials/M_WorldGrid.M_WorldGrid"));
}

void ADefenseEntity::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->AddLooseGameplayTag(MetaTags::State_Invulnerable);
	MeshBaseRotation = GetMesh()->GetRelativeRotation();
	if (UMaterialInterface* Mat = GridMaterial.LoadSynchronous())
	{
		GridMID = UMaterialInstanceDynamic::Create(Mat, this);
		for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
		{
			GetMesh()->SetMaterial(i, GridMID);
		}
	}
}

void ADefenseEntity::SlideTo(FVector Target, float Seconds)
{
	SlideFrom = GetActorLocation();
	SlideTarget = FVector(Target.X, Target.Y, SlideFrom.Z);
	SlideDuration = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	SlideElapsed = 0.f;
	SetActorRotation((SlideTarget - SlideFrom).GetSafeNormal2D().Rotation());
}

void ADefenseEntity::ChoppySwing()
{
	SwingElapsed = 0.f;
}

void ADefenseEntity::SetExplode(float Amount)
{
	if (GridMID)
	{
		GridMID->SetScalarParameterValue(TEXT("ExplodeAmount"), Amount);
	}
}

void ADefenseEntity::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (SlideElapsed >= 0.f)
	{
		SlideElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(SlideElapsed / SlideDuration, 0.f, 1.f);
		SetActorLocation(FMath::Lerp(SlideFrom, SlideTarget, Alpha), true);
		if (Alpha >= 1.f)
		{
			SlideElapsed = -1.f;
		}
	}
	if (SwingElapsed >= 0.f)
	{
		SwingElapsed += DeltaSeconds;
		const int32 Frame = FMath::FloorToInt32(SwingElapsed / SwingSeconds * UE_ARRAY_COUNT(SwingPoses));
		if (Frame >= static_cast<int32>(UE_ARRAY_COUNT(SwingPoses)))
		{
			GetMesh()->SetRelativeRotation(MeshBaseRotation);
			SwingElapsed = -1.f;
		}
		else
		{
			GetMesh()->SetRelativeRotation(MeshBaseRotation + SwingPoses[Frame]);
		}
	}
}
