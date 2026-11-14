#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "MetaSettingsWidget.generated.h"

class UBorder;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class USlider;
class UComboBoxString;
class UCheckBox;

/**
 * WBP_Settings: the virtual settings menu opened with ESC (spec 6.1). Every change applies immediately through
 * UMetaSettingsSubsystem; values forced by bosses are reflected live. At 8 deaths the answer row blinks red (6.3).
 */
UCLASS()
class ACCESSEDNONE_API UMetaSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Keys whose rows currently blink (for tests / BP). */
	UFUNCTION(BlueprintPure) TArray<EMetaSettingKey> GetBlinkingKeys() const { return BlinkKeys; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	struct FRow
	{
		EMetaSettingKey Key;
		TObjectPtr<UBorder> Border;
		TObjectPtr<USlider> Slider;
		TObjectPtr<UComboBoxString> Combo;
		TObjectPtr<UCheckBox> Check;
		TObjectPtr<UTextBlock> ValueText;
	};

	void AddSlider(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, float Min, float Max, float Step);
	void AddCombo(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, const TArray<FText>& Options);
	void AddToggle(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey);
	FRow& AddRow(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, UWidget* Control);
	void SyncRow(FRow& Row, float Value);
	float ReadControl(const FRow& Row) const;
	void PushControlsToSettings();
	void HandleSettingChanged(EMetaSettingKey Key, float Old, float New);
	void RefreshBlinkKeys();
	UMetaSettingsSubsystem* Settings() const;

	UFUNCTION() void HandleSlider(float Value) { PushControlsToSettings(); }
	UFUNCTION() void HandleCombo(FString Item, ESelectInfo::Type Info) { PushControlsToSettings(); }
	UFUNCTION() void HandleCheck(bool bChecked) { PushControlsToSettings(); }
	UFUNCTION() void ShowTab0();
	UFUNCTION() void ShowTab1();
	UFUNCTION() void ShowTab2();

	UPROPERTY() TObjectPtr<UWidgetSwitcher> Tabs;
	TArray<FRow> Rows;
	TArray<EMetaSettingKey> BlinkKeys;
	FDelegateHandle SettingHandle;
	bool bSyncing = false;
};
