#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaHUDWidget.generated.h"

class UCanvasPanel;
class UProgressBar;
class UTextBlock;
class UBorder;
class UVerticalBox;
class UMetaBossBarWidget;

/**
 * WBP_HUD: souls-like HUD (spec 4.1) plus the screen-space pieces the story needs:
 * floating damage, toast (Access Denied), center banner, system log lines, tutorial popup, screen tint.
 */
UCLASS()
class ACCESSEDNONE_API UMetaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable) void ShowToast(const FText& Text, float Seconds = 2.f);
	UFUNCTION(BlueprintCallable) void ShowFloatingText(FVector WorldLocation, const FText& Text, FLinearColor Color, float Size = 22.f);
	/** Big centered text. Empty text hides it. */
	UFUNCTION(BlueprintCallable) void ShowBanner(const FText& Text, FLinearColor Color, float Size = 64.f);
	/** Shakes the banner and swaps to GlitchedText over Seconds (spec 4.2 "UMG glitch"). */
	UFUNCTION(BlueprintCallable) void GlitchBanner(const FText& GlitchedText, float Seconds = 0.6f);
	UFUNCTION(BlueprintCallable) void PushLogLine(const FText& Line, FLinearColor Color);
	UFUNCTION(BlueprintCallable) void ClearLog();
	/** Empty text hides the popup. */
	UFUNCTION(BlueprintCallable) void ShowTutorial(const FText& Text);
	UFUNCTION(BlueprintCallable) void SetScreenTint(FLinearColor Color);
	/** Hides HP/stamina/quick slot/boss bar (death screen). */
	UFUNCTION(BlueprintCallable) void SetStatusVisible(bool bVisible);
	/** Scripted hits show their own text (4.4 "-999,999,999"). */
	UFUNCTION(BlueprintCallable) void SetDamageNumbersEnabled(bool bEnabled) { bDamageNumbers = bEnabled; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FFloating
	{
		TWeakObjectPtr<UTextBlock> Widget;
		FVector WorldLocation;
		double StartTime = 0.0;
	};

	void HandleDamage(AActor* Target, float Damage, bool bImmune);

	UPROPERTY() TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY() TObjectPtr<UWidget> StatusRoot;
	UPROPERTY() TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY() TObjectPtr<UProgressBar> HealthTrail;
	UPROPERTY() TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY() TObjectPtr<UProgressBar> StaminaTrail;
	UPROPERTY() TArray<TObjectPtr<class UImage>> FlaskIcons;
	UPROPERTY() TObjectPtr<UTextBlock> FlaskText;
	UPROPERTY() TObjectPtr<UMetaBossBarWidget> BossBar;
	UPROPERTY() TObjectPtr<UTextBlock> ToastText;
	UPROPERTY() TObjectPtr<UTextBlock> BannerText;
	UPROPERTY() TObjectPtr<UVerticalBox> LogBox;
	UPROPERTY() TObjectPtr<UBorder> TutorialBox;
	UPROPERTY() TObjectPtr<UTextBlock> TutorialText;
	UPROPERTY() TObjectPtr<UBorder> Tint;

	TArray<FFloating> Floating;
	FDelegateHandle DamageHandle;
	double ToastEndTime = 0.0;
	double GlitchStart = 0.0;
	double GlitchEnd = 0.0;
	FText GlitchTarget;
	bool bStatusVisible = true;
	bool bDamageNumbers = true;
	double HealthTrailHold = 0.0;
	double StaminaTrailHold = 0.0;
	static constexpr float HealthBarWidth = 460.f;
	static constexpr float StaminaBarWidth = 360.f;
};
