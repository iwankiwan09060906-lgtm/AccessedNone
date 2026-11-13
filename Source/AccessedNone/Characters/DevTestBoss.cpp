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

	// 7.5 placeholder: T-pose mannequin (no anim instance).
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshFinder.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	}
}
