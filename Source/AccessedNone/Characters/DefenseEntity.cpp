#include "Characters/DefenseEntity.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** UE4-mannequin bone names (the Infinity Blade warriors use that skeleton) and the swing range in degrees. */
	struct FPoseBone { const TCHAR* Name; float Swing; float Twist; };
	const FPoseBone PoseBones[] = {
		{ TEXT("spine_01"), 20.f, 15.f }, { TEXT("spine_03"), 20.f, 20.f }, { TEXT("neck_01"), 25.f, 30.f },
		{ TEXT("clavicle_l"), 20.f, 0.f }, { TEXT("clavicle_r"), 20.f, 0.f },
		{ TEXT("upperarm_l"), 75.f, 40.f }, { TEXT("upperarm_r"), 75.f, 40.f },
		{ TEXT("lowerarm_l"), 60.f, 20.f }, { TEXT("lowerarm_r"), 60.f, 20.f },
		{ TEXT("hand_l"), 35.f, 30.f }, { TEXT("hand_r"), 35.f, 30.f },
		{ TEXT("thigh_l"), 45.f, 20.f }, { TEXT("thigh_r"), 45.f, 20.f },
		{ TEXT("calf_l"), 55.f, 0.f }, { TEXT("calf_r"), 55.f, 0.f } };
	constexpr float FallSpinMin = 1.5f;   // rad/s
	constexpr float FallSpinMax = 5.f;
	constexpr float BlastSpin = 9.f;
	constexpr float LandSettleRate = 8.f;
}

ADefenseEntity::ADefenseEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true; // fall without a controller
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetVisibility(false); // the posed body below is what we see

	Body = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetRelativeLocationAndRotation(BodyOffset, FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (const TCHAR* Name : { TEXT("SK_CharM_Barbarous"), TEXT("SK_CharM_Bladed"), TEXT("SK_CharM_Cardboard"), TEXT("SK_CharM_Forge"),
		TEXT("SK_CharM_FrostGiant"), TEXT("SK_CharM_Golden"), TEXT("SK_CharM_Natural"), TEXT("SK_CharM_Pit"), TEXT("SK_CharM_Ragged0"),
		TEXT("SK_CharM_RaggedElite"), TEXT("SK_CharM_Ram"), TEXT("SK_CharM_Robo"), TEXT("SK_CharM_Shell"), TEXT("SK_CharM_solid"),
		TEXT("SK_CharM_Standard"), TEXT("SK_CharM_Tusk"), TEXT("SK_CharM_Warrior") })
	{
		Meshes.Add(TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/InfinityBladeWarriors/Character/CompleteCharacters/%s.%s"), Name, Name))));
	}
}

void ADefenseEntity::BeginPlay()
{
	if (!Meshes.IsEmpty())
	{
		Body->SetSkinnedAssetAndUpdate(Meshes[FMath::RandRange(0, Meshes.Num() - 1)].LoadSynchronous());
	}
	Super::BeginPlay();
	AbilitySystem->AddLooseGameplayTag(MetaTags::State_Invulnerable);
	BodyBaseRotation = Body->GetRelativeRotation().Quaternion();
	ApplyRandomPose();
	SpinAxis = FMath::VRand();
	SpinSpeed = FMath::FRandRange(FallSpinMin, FallSpinMax);
}

void ADefenseEntity::ApplyRandomPose()
{
	const USkinnedAsset* Asset = Body->GetSkinnedAsset();
	if (!Asset)
	{
		return;
	}
	const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
	Body->BoneSpaceTransforms = Ref.GetRefBonePose();
	for (const FPoseBone& B : PoseBones)
	{
		const int32 Index = Ref.FindBoneIndex(B.Name);
		if (Index == INDEX_NONE)
		{
			continue;
		}
		const FRotator Delta(FMath::FRandRange(-B.Swing, B.Swing), FMath::FRandRange(-B.Twist, B.Twist), FMath::FRandRange(-B.Swing, B.Swing));
		FTransform& Local = Body->BoneSpaceTransforms[Index];
		Local.SetRotation(Local.GetRotation() * Delta.Quaternion());
	}
	Body->MarkRefreshTransformDirty();
}

void ADefenseEntity::BlastAway(FVector Origin, float Strength)
{
	const FVector Away = (GetActorLocation() - Origin).GetSafeNormal2D();
	LaunchCharacter(Away * Strength + FVector(0.f, 0.f, Strength * 0.8f), true, true);
	SpinAxis = FMath::VRand();
	SpinSpeed = BlastSpin;
}

void ADefenseEntity::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetCharacterMovement()->IsFalling())
	{
		Tumble = FQuat(SpinAxis, SpinSpeed * DeltaSeconds) * Tumble;
	}
	else
	{
		Tumble = FQuat::Slerp(Tumble, FQuat::Identity, FMath::Min(1.f, DeltaSeconds * LandSettleRate)); // land on its feet, pose kept
	}
	// Spin about the capsule center (the mesh origin is at the feet).
	Body->SetRelativeLocationAndRotation(Tumble.RotateVector(BodyOffset), Tumble * BodyBaseRotation);
}
