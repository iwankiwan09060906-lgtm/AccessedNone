#include "Characters/DefenseEntity.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ADefenseEntity::ADefenseEntity()
{
	AutoPossessAI = EAutoPossessAI::Disabled;
	GetCharacterMovement()->bRunPhysicsWithNoController = true; // fall without a controller
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetAnimInstanceClass(nullptr); // reference pose = T-pose
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

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
		GetMesh()->SetSkeletalMeshAsset(Meshes[FMath::RandRange(0, Meshes.Num() - 1)].LoadSynchronous());
	}
	Super::BeginPlay();
	AbilitySystem->AddLooseGameplayTag(MetaTags::State_Invulnerable);
}

void ADefenseEntity::BlastAway(FVector Origin, float Strength)
{
	const FVector Away = (GetActorLocation() - Origin).GetSafeNormal2D();
	LaunchCharacter(Away * Strength + FVector(0.f, 0.f, Strength * 0.8f), true, true);
	SetActorRotation(FRotator(FMath::FRandRange(-60.f, 60.f), GetActorRotation().Yaw, FMath::FRandRange(-60.f, 60.f)));
}
