#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "MetaSettingsWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class USlider;
class UComboBoxString;
class UCheckBox;

/**
 * WBP_Settings: the virtual settings menu opened with ESC (spec 6.1). Every change applies immediately through
 * UMetaSettingsSubsystem; values forced by bosses are reflected live. At 8 deaths the answer row blinks red (6.3).
 * The four gimmick rows hide among many filler rows (decoys: remembered, never applied to the real system) across
 * seven tabs, plus a key-binding page and Reset / Main menu / Quit / Close buttons.
 */
UCLASS()
class ACCESSEDNONE_API UMetaSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Keys whose rows currently blink (for tests / BP). */
	UFUNCTION(BlueprintPure) TArray<EMetaSettingKey> GetBlinkingKeys() const { return BlinkKeys; }
	/** Number of rows (real + decoy) and tabs, for tests. */
	int32 GetRowCount() const { return Rows.Num(); }
	int32 GetTabCount() const { return TabButtons.Num(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum class EFormat : uint8 { None, Percent, OneDecimal, Integer, Balance };

	/** A real gimmick-linked setting, or a decoy stored under its label key. */
	struct FRef
	{
		bool bReal = false;
		EMetaSettingKey Key = EMetaSettingKey::Gamma;
		FName Decoy;
		float Default = 0.f;
	};
	static FRef Real(EMetaSettingKey Key) { FRef R; R.bReal = true; R.Key = Key; return R; }
	static FRef Decoy(const TCHAR* LabelKey, float Default) { FRef R; R.Decoy = LabelKey; R.Default = Default; return R; }

	struct FRow
	{
		FRef Ref;
		EFormat Format = EFormat::None;
		TObjectPtr<UBorder> Border;
		TObjectPtr<USlider> Slider;
		TObjectPtr<UComboBoxString> Combo;
		TObjectPtr<UCheckBox> Check;
		TObjectPtr<UTextBlock> ValueText;
	};

	struct FBindRow
	{
		FName Id;
		FKey Default;
		TObjectPtr<UButton> Button;
		TObjectPtr<UTextBlock> KeyText;
	};

	UVerticalBox* AddPage(const TCHAR* TabKey);
	void AddSlider(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, float Min, float Max, float Step, EFormat Format);
	void AddCombo(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, const TArray<FText>& Options);
	void AddToggle(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey);
	void AddBinding(UVerticalBox* Page, const TCHAR* LabelKey, const FKey& Default);
	FRow& AddRow(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, UWidget* Control);
	UButton* MakeButton(const FText& Label, int32 Size);

	float ReadSetting(const FRow& Row) const;
	void SyncRow(FRow& Row, float Value);
	void SyncAll();
	float ReadControl(const FRow& Row) const;
	void PushControlsToSettings();
	void HandleSettingChanged(EMetaSettingKey Key, float Old, float New);
	void RefreshBlinkKeys();
	void RefreshBinding(FBindRow& Row);
	void ShowTab(int32 Index);
	UMetaSettingsSubsystem* Settings() const;

	UFUNCTION() void HandleSlider(float Value) { PushControlsToSettings(); }
	UFUNCTION() void HandleCombo(FString Item, ESelectInfo::Type Info) { PushControlsToSettings(); }
	UFUNCTION() void HandleCheck(bool bChecked) { PushControlsToSettings(); }
	UFUNCTION() void HandleTabClicked();
	UFUNCTION() void HandleBindClicked();
	UFUNCTION() void HandleReset();
	UFUNCTION() void HandleMainMenu();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleClose();

	UPROPERTY() TObjectPtr<UHorizontalBox> TabBar;
	UPROPERTY() TObjectPtr<UWidgetSwitcher> Tabs;
	UPROPERTY() TArray<TObjectPtr<UButton>> TabButtons;
	TArray<FRow> Rows;
	TArray<FBindRow> Bindings;
	TArray<EMetaSettingKey> BlinkKeys;
	FDelegateHandle SettingHandle;
	int32 ListeningBind = INDEX_NONE;
	bool bSyncing = false;
};
