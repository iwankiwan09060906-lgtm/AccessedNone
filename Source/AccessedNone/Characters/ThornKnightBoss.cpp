#include "Characters/ThornKnightBoss.h"
#include "Core/MetaText.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaGameplayTags.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayEffects.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Gimmick/GimmickCondition.h"
#include "UI/MetaUI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkinnedAsset.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 SpikeCount = 26;

	/** One box of the low-poly knight. End bone set: spans Bone → End. Otherwise a box of Size centered at Bone + Offset. */
	struct FBlockSpec
	{
		const TCHAR* Bone;
		const TCHAR* End;
		FVector Size;   // cm in mesh space; Z ignored for spans (= bone length + padding)
		FVector Offset; // mesh space (Grux faces +Y)
	};
	// Bone names of the Paragon Grux skeleton (DA_Boss_ThornKnight).
	const FBlockSpec BlockSpecs[] = {
		{ TEXT("pelvis"), nullptr, FVector(85, 70, 50), FVector(0, 0, -5) },
		{ TEXT("spine_01"), TEXT("neck_01"), FVector(100, 80, 0), FVector::ZeroVector },
		{ TEXT("head"), nullptr, FVector(52, 58, 52), FVector(0, 6, 4) },
		{ TEXT("upperarm_l"), nullptr, FVector(58, 58, 44), FVector(0, 0, 10) },
		{ TEXT("upperarm_r"), nullptr, FVector(58, 58, 44), FVector(0, 0, 10) },
		{ TEXT("upperarm_l"), TEXT("lowerarm_l"), FVector(40, 40, 0), FVector::ZeroVector },
		{ TEXT("upperarm_r"), TEXT("lowerarm_r"), FVector(40, 40, 0), FVector::ZeroVector },
		{ TEXT("lowerarm_l"), TEXT("hand_l"), FVector(36, 36, 0), FVector::ZeroVector },
		{ TEXT("lowerarm_r"), TEXT("hand_r"), FVector(36, 36, 0), FVector::ZeroVector },
		{ TEXT("hand_l"), nullptr, FVector(34, 34, 34), FVector(0, 0, -12) },
		{ TEXT("hand_r"), nullptr, FVector(34, 34, 34), FVector(0, 0, -12) },
		{ TEXT("thigh_l"), TEXT("calf_l"), FVector(44, 44, 0), FVector::ZeroVector },
		{ TEXT("thigh_r"), TEXT("calf_r"), FVector(44, 44, 0), FVector::ZeroVector },
		{ TEXT("calf_l"), TEXT("foot_l"), FVector(38, 38, 0), FVector::ZeroVector },
		{ TEXT("calf_r"), TEXT("foot_r"), FVector(38, 38, 0), FVector::ZeroVector },
		{ TEXT("foot_l"), nullptr, FVector(36, 56, 20), FVector(0, 12, 8) },
		{ TEXT("foot_r"), nullptr, FVector(36, 56, 20), FVector(0, 12, 8) },
	};
	constexpr float BlockSpanPadding = 14.f;
}

AThornKnightBoss::AThornKnightBoss()
{
	PrimaryActorTick.bCanEverTick = true;
	UGimmickCondition_SettingThreshold* Quality = CreateDefaultSubobject<UGimmickCondition_SettingThreshold>(TEXT("MeshQualityCondition"));
	Quality->Key = EMetaSettingKey::MeshQuality;
	Quality->Threshold = 0.f;
	Quality->bEquals = true;
	Gimmick->Conditions.Add(Quality);
	Gimmick->SettingsToResetOnStart.Add(EMetaSettingKey::MeshQuality, 3.f);
	Gimmick->BreakingDuration = 1.f;

	// Thorns on the upper body, pointing outward (mesh space: Z up from the feet).
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Grid(TEXT("/Game/Core/Materials/M_WorldGrid.M_WorldGrid"));
	BlockMesh = Cube.Object;
	BlockMaterial = Grid.Object; // gray checker of the defense entities (spec 12.1)
	FRandomStream Rng(48000000);
	for (int32 i = 0; i < SpikeCount; ++i)
	{
		UStaticMeshComponent* Spike = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Spike_Hitbox_%d"), i));
		Spike->SetupAttachment(GetMesh());
		if (Cone.Succeeded()) { Spike->SetStaticMesh(Cone.Object); }
		const float Yaw = Rng.FRandRange(0.f, UE_TWO_PI);
		const float Up = Rng.FRandRange(-0.35f, 0.9f);
		const FVector Normal = FVector(FMath::Cos(Yaw) * (1.f - FMath::Abs(Up)), FMath::Sin(Yaw) * (1.f - FMath::Abs(Up)), Up).GetSafeNormal();
		const FVector Center(0.f, 0.f, 150.f);
		Spike->SetRelativeLocation(Center + Normal * FVector(55.f, 45.f, 60.f));
		Spike->SetRelativeRotation(FRotationMatrix::MakeFromZ(Normal).Rotator());
		Spike->SetRelativeScale3D(FVector(0.18f, 0.18f, Rng.FRandRange(0.45f, 0.8f)));
		Spike->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
		Spike->SetGenerateOverlapEvents(false);
		Spike->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Spikes.Add(Spike);
	}

	TrisText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TrisText"));
	TrisText->SetupAttachment(RootComponent);
	TrisText->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	TrisText->SetHorizontalAlignment(EHTA_Center);
	TrisText->SetWorldSize(22.f);
	TrisText->SetTextRenderColor(FColor::FromHex(TEXT("E5243B")));
}

void AThornKnightBoss::BeginPlay()
{
	Super::BeginPlay();
	TrisText->SetText(MetaText::Get(TEXT("E2_DEBUG_TRIS")));
	for (UStaticMeshComponent* Spike : Spikes)
	{
		SpikeScales.Add(Spike->GetRelativeScale3D());
		if (UMaterialInstanceDynamic* MID = Spike->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.06f, 0.05f, 0.05f)); // dark iron
		}
	}
	BuildBlockBody();
	SetHighPoly();
	DamageHandle = FMetaCombatEvents::OnDamage.AddUObject(this, &AThornKnightBoss::HandleAnyDamage);
}

void AThornKnightBoss::EndPlay(const EEndPlayReason::Type Reason)
{
	FMetaCombatEvents::OnDamage.Remove(DamageHandle);
	Super::EndPlay(Reason);
}

void AThornKnightBoss::HandleAnyDamage(AActor* Target, float Damage, bool bImmune)
{
	// Only the player ever hits a boss; the thorns answer every hit while the armor is up.
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Target != this || bLowPoly || IsDead() || !Player || Gimmick->GetState() != EGimmickState::Armored)
	{
		return;
	}
	UAbilitySystemComponent* PlayerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Player);
	if (!PlayerASC)
	{
		return;
	}
	++ThornStrikes;
	ThornBurstLeft = ThornBurstSeconds;
	const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, UAccessedNoneSettings::Get().BossSkillDamage);
	AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data, PlayerASC);
}

void AThornKnightBoss::SetHighPoly()
{
	bLowPoly = false;
	DissolveElapsed = -1.f;
	for (int32 i = 0; i < Spikes.Num(); ++i)
	{
		Spikes[i]->SetRelativeScale3D(SpikeScales.IsValidIndex(i) ? SpikeScales[i] : Spikes[i]->GetRelativeScale3D());
		Spikes[i]->SetVisibility(true);
		Spikes[i]->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	GetMesh()->SetVisibility(true, false);
	SetBlockAlpha(0.f);
	TrisText->SetVisibility(true);
}

void AThornKnightBoss::FinishLowPoly()
{
	bLowPoly = true;
	DissolveElapsed = -1.f;
	for (UStaticMeshComponent* Spike : Spikes)
	{
		Spike->SetVisibility(false);
		Spike->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetBlockAlpha(1.f);
	GetMesh()->SetVisibility(false, false); // keeps animating (AlwaysTickPoseAndRefreshBones) so the boxes move
	TrisText->SetVisibility(false);
}

void AThornKnightBoss::BuildBlockBody()
{
	const USkinnedAsset* Asset = GetMesh()->GetSkinnedAsset();
	if (!Asset || !BlockMesh || !Blocks.IsEmpty())
	{
		return;
	}
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	const FReferenceSkeleton& Ref = Asset->GetRefSkeleton();
	auto RefPose = [&Ref](const TCHAR* Bone, bool& bFound)
	{
		FTransform T = FTransform::Identity;
		int32 Index = Ref.FindBoneIndex(Bone);
		bFound = Index != INDEX_NONE;
		for (; Index != INDEX_NONE; Index = Ref.GetParentIndex(Index))
		{
			T = T * Ref.GetRefBonePose()[Index];
		}
		return T;
	};

	for (const FBlockSpec& Spec : BlockSpecs)
	{
		bool bBone = false, bEnd = true;
		const FTransform BoneCS = RefPose(Spec.Bone, bBone);
		FTransform BoxCS(FQuat::Identity, BoneCS.GetLocation() + Spec.Offset, Spec.Size / 100.f); // the engine cube is 100 cm
		if (Spec.End)
		{
			const FVector A = BoneCS.GetLocation();
			const FVector B = RefPose(Spec.End, bEnd).GetLocation();
			BoxCS = FTransform(FRotationMatrix::MakeFromZ(B - A).ToQuat(), (A + B) * 0.5f + Spec.Offset,
				FVector(Spec.Size.X, Spec.Size.Y, FVector::Dist(A, B) + BlockSpanPadding) / 100.f);
		}
		if (!bBone || !bEnd)
		{
			continue; // a different skeleton: skip boxes it can't place
		}
		UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("LowPolyBlock_%d"), Blocks.Num()));
		Block->SetStaticMesh(BlockMesh);
		if (BlockMaterial) { Block->SetMaterial(0, BlockMaterial); }
		Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Block->SetupAttachment(GetMesh(), Spec.Bone);
		Block->SetRelativeTransform(BoxCS.GetRelativeTransform(BoneCS));
		Block->RegisterComponent();
		Blocks.Add(Block);
		BlockScales.Add(Block->GetRelativeScale3D());
	}
}

void AThornKnightBoss::SetBlockAlpha(float Alpha)
{
	for (int32 i = 0; i < Blocks.Num(); ++i)
	{
		Blocks[i]->SetVisibility(Alpha > 0.f);
		Blocks[i]->SetRelativeScale3D(BlockScales[i] * FMath::Max(Alpha, KINDA_SMALL_NUMBER));
	}
}

void AThornKnightBoss::HandleGimmickStateChanged(EGimmickState Old, EGimmickState New)
{
	Super::HandleGimmickStateChanged(Old, New);
	if (New == EGimmickState::Dormant)
	{
		SetHighPoly();
	}
	else if (New == EGimmickState::Armored && Old == EGimmickState::Dormant)
	{
		SetHighPoly();
		PlayFrameDrop();
	}
	else if (New == EGimmickState::Breaking)
	{
		DissolveElapsed = 0.f;
		for (UStaticMeshComponent* Spike : Spikes)
		{
			Spike->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AThornKnightBoss::PlayFrameDrop()
{
	// "The frame rate tanks as it appears": a real render-rate cap for a moment (game setting, not the OS).
	IConsoleVariable* MaxFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
	if (!MaxFPS)
	{
		return;
	}
	const float Previous = MaxFPS->GetFloat();
	MaxFPS->Set(static_cast<float>(FrameDropFPS), ECVF_SetByCode);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([MaxFPS, Previous](float) { MaxFPS->Set(Previous, ECVF_SetByCode); return false; }), FrameDropSeconds);
}

void AThornKnightBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DissolveElapsed >= 0.f)
	{
		DissolveElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(DissolveElapsed / DissolveSeconds, 0.f, 1.f);
		for (int32 i = 0; i < Spikes.Num() && i < SpikeScales.Num(); ++i)
		{
			Spikes[i]->SetRelativeScale3D(SpikeScales[i] * (1.f - Alpha));
		}
		SetBlockAlpha(Alpha);
		if (Alpha >= 1.f)
		{
			FinishLowPoly();
		}
	}
	if (ThornBurstLeft > 0.f && DissolveElapsed < 0.f && !bLowPoly)
	{
		// Thorn burst: the spikes shoot out and snap back.
		ThornBurstLeft = FMath::Max(0.f, ThornBurstLeft - DeltaSeconds);
		const float Pulse = FMath::Sin(UE_PI * (1.f - ThornBurstLeft / ThornBurstSeconds));
		for (int32 i = 0; i < Spikes.Num() && i < SpikeScales.Num(); ++i)
		{
			Spikes[i]->SetRelativeScale3D(SpikeScales[i] * FVector(1.f, 1.f, 1.f + (ThornBurstScale - 1.f) * Pulse));
		}
	}
	if (TrisText->IsVisible())
	{
		// Face the camera; from hint tier 1 the counter throbs ("이상 징후 강화").
		if (const APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			TrisText->SetWorldRotation((Cam->GetCameraLocation() - TrisText->GetComponentLocation()).GetSafeNormal2D().Rotation());
		}
		const float Pulse = HintTier >= 1 ? 1.f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) : 1.f;
		TrisText->SetWorldSize(22.f * Pulse);
	}
}
