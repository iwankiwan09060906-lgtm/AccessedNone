#include "Presentation/MetaPostProcessController.h"
#include "Components/PostProcessComponent.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "Misc/App.h"

AMetaPostProcessController::AMetaPostProcessController()
{
	PrimaryActorTick.bCanEverTick = true;
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
	PostProcess->bUnbound = true;
	PostProcess->Priority = 10.f;
	RootComponent = PostProcess;
}

void AMetaPostProcessController::BeginPlay()
{
	Super::BeginPlay();
	UMetaSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>();
	SettingHandle = Settings->OnSettingChangedNative.AddUObject(this, &AMetaPostProcessController::HandleSettingChanged);
	TargetGamma = AppliedGamma = Settings->GetValue(EMetaSettingKey::Gamma);
	ApplyGamma(AppliedGamma);
	ApplyStatic(EMetaSettingKey::MotionBlur, Settings->GetValue(EMetaSettingKey::MotionBlur));
	ApplyStatic(EMetaSettingKey::MasterVolume, Settings->GetValue(EMetaSettingKey::MasterVolume));
}

void AMetaPostProcessController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UMetaSettingsSubsystem>()->OnSettingChangedNative.Remove(SettingHandle);
	}
	Super::EndPlay(Reason);
}

void AMetaPostProcessController::HandleSettingChanged(EMetaSettingKey Key, float Old, float New)
{
	if (Key == EMetaSettingKey::Gamma)
	{
		TargetGamma = New;
	}
	else
	{
		ApplyStatic(Key, New);
	}
}

void AMetaPostProcessController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (AppliedGamma != TargetGamma)
	{
		// Real delta: the menu slows the world to x0.15, the picture must still follow the slider.
		AppliedGamma = FMath::FInterpConstantTo(AppliedGamma, TargetGamma, static_cast<float>(FApp::GetDeltaTime()), 1.f / FullSweepSeconds);
		ApplyGamma(AppliedGamma);
	}
}

void AMetaPostProcessController::ApplyGamma(float Gamma)
{
	FPostProcessSettings& S = PostProcess->Settings;
	const float Offset = Gamma - 0.5f;
	const bool bDark = Offset < 0.f;
	float EV = Offset * 2.f * (bDark ? DarkExposureRange : GammaExposureRange);
	if (Gamma >= WhiteoutGamma)
	{
		EV += WhiteoutExtraEV * FMath::GetRangePct(WhiteoutGamma, 1.f, Gamma);
	}
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = EV;
	S.bOverride_ColorGamma = true;
	S.ColorGamma = FVector4(FVector(1.f + Offset * 2.f * (bDark ? DarkCurveRange : GammaCurveRange)), 1.f);
}

void AMetaPostProcessController::ApplyStatic(EMetaSettingKey Key, float Value)
{
	if (Key == EMetaSettingKey::MotionBlur)
	{
		PostProcess->Settings.bOverride_MotionBlurAmount = true;
		PostProcess->Settings.MotionBlurAmount = Value > 0.5f ? MotionBlurAmount : 0.f;
	}
	else if (Key == EMetaSettingKey::MasterVolume)
	{
		// In-game mix only (spec 1.3: OS audio untouched).
		if (FAudioDevice* Audio = GetWorld()->GetAudioDeviceRaw())
		{
			Audio->SetTransientPrimaryVolume(Value);
		}
	}
}
