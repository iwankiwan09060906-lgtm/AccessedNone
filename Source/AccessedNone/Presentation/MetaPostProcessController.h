#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "MetaPostProcessController.generated.h"

class UPostProcessComponent;

/**
 * One per level (AMetaGameMode spawns it if the level has none). Applies the virtual settings that change the
 * picture or the mix (spec 8.9): Gamma → ColorGamma + AutoExposureBias, MotionBlur, MasterVolume.
 * Never touches real OS / GameUserSettings (spec 1.3).
 */
UCLASS()
class ACCESSEDNONE_API AMetaPostProcessController : public AActor
{
	GENERATED_BODY()

public:
	AMetaPostProcessController();
	virtual void Tick(float DeltaSeconds) override;

	/** Gamma 0.5 = neutral. ±0.5 maps to ±GammaExposureRange EV; ≥ WhiteoutGamma adds WhiteoutExtraEV (E1). */
	UPROPERTY(EditAnywhere, Category="Gamma") float GammaExposureRange = 3.f;
	UPROPERTY(EditAnywhere, Category="Gamma") float GammaCurveRange = 0.6f;
	/** Below 0.5 the picture darkens more gently: E1's 20 % is a dark forest with silhouettes, not a black screen. */
	UPROPERTY(EditAnywhere, Category="Gamma") float DarkExposureRange = 1.8f;
	UPROPERTY(EditAnywhere, Category="Gamma") float DarkCurveRange = 0.35f;
	UPROPERTY(EditAnywhere, Category="Gamma") float WhiteoutGamma = 0.95f;
	UPROPERTY(EditAnywhere, Category="Gamma") float WhiteoutExtraEV = 3.f;
	/** Fastest change from darkest to whiteout (spec 12.3: the E1 whiteout takes ≥ 0.8 s). */
	UPROPERTY(EditAnywhere, Category="Gamma") float FullSweepSeconds = 0.8f;
	UPROPERTY(EditAnywhere, Category="MotionBlur") float MotionBlurAmount = 0.5f;

	UFUNCTION(BlueprintPure) float GetAppliedGamma() const { return AppliedGamma; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void HandleSettingChanged(EMetaSettingKey Key, float Old, float New);
	void ApplyGamma(float Gamma);
	void ApplyStatic(EMetaSettingKey Key, float Value);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> PostProcess;
	FDelegateHandle SettingHandle;
	float TargetGamma = 0.5f;
	float AppliedGamma = 0.5f;
};
