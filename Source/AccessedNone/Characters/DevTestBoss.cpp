#include "Characters/DevTestBoss.h"
#include "Gimmick/GimmickCondition.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ADevTestBoss::ADevTestBoss()
{
	UGimmickCondition_SettingThreshold* Gamma = CreateDefaultSubobject<UGimmickCondition_SettingThreshold>(TEXT("GammaCondition"));
	Gamma->Key = EMetaSettingKey::Gamma;
	Gamma->Threshold = 0.95f;
	Gamma->bGreaterOrEqual = true;
	Gimmick->Conditions.Add(Gamma);
	Gimmick->SettingsToResetOnStart.Add(EMetaSettingKey::Gamma, 0.2f);

	// Test boss = Paragon Narbash (skeleton + anim BP; locomotion follows velocity).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/ParagonNarbash/Characters/Heroes/Narbash/Meshes/Narbash.Narbash"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimFinder(TEXT("/Game/ParagonNarbash/Characters/Heroes/Narbash/Narbash_AnimBlueprint"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshFinder.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	}
	if (AnimFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimFinder.Class);
	}
}
