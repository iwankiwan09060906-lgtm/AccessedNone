#include "Characters/ThornKnightBoss.h"
#include "Core/MetaText.h"
#include "Gimmick/GimmickCondition.h"
#include "UI/MetaUI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 SpikeCount = 26;
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
	SetHighPoly();
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
	GetMesh()->SetForcedLOD(0);
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
	GetMesh()->SetForcedLOD(GetMesh()->GetNumLODs()); // lowest LOD = the low-poly knight
	TrisText->SetVisibility(false);
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
		if (Alpha >= 1.f)
		{
			FinishLowPoly();
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
