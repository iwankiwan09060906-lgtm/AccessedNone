#include "UI/MetaSettingsWidget.h"
#include "UI/MetaUI.h"
#include "Characters/MetaBossBase.h"
#include "Core/MetaText.h"
#include "Gimmick/GimmickCondition.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

namespace
{
	constexpr int32 BlinkTier = 8;      // spec 6.3
	constexpr double BlinkHz = 1.0;     // well under the 3 Hz photosensitivity limit (12.3)
	constexpr float LabelWidth = 300.f;
	constexpr float ControlWidth = 300.f;
}

UMetaSettingsSubsystem* UMetaSettingsWidget::Settings() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>() : nullptr;
}

TSharedRef<SWidget> UMetaSettingsWidget::RebuildWidget()
{
	if (!MetaUI::NeedsBuild(WidgetTree))
	{
		return Super::RebuildWidget();
	}

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Backdrop->SetHorizontalAlignment(HAlign_Center);
	Backdrop->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Backdrop;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(MetaUI::Panel);
	Panel->SetPadding(FMargin(32.f, 24.f));
	Backdrop->SetContent(Panel);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Box);
	Box->AddChildToVerticalBox(MetaUI::Text(WidgetTree, MetaText::Get(TEXT("SET_TITLE")), 30, MetaUI::Gold, TEXT("Bold")));

	UHorizontalBox* TabBar = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(TabBar)->SetPadding(FMargin(0.f, 14.f));
	const TCHAR* TabKeys[] = { TEXT("SET_TAB_GRAPHICS"), TEXT("SET_TAB_AUDIO"), TEXT("SET_TAB_CONTROLS") };
	for (int32 i = 0; i < 3; ++i)
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>();
		B->SetContent(MetaUI::Text(WidgetTree, MetaText::Get(TabKeys[i]), 18, FLinearColor::Black));
		if (i == 0) { B->OnClicked.AddDynamic(this, &UMetaSettingsWidget::ShowTab0); }
		if (i == 1) { B->OnClicked.AddDynamic(this, &UMetaSettingsWidget::ShowTab1); }
		if (i == 2) { B->OnClicked.AddDynamic(this, &UMetaSettingsWidget::ShowTab2); }
		TabBar->AddChildToHorizontalBox(B)->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	Tabs = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Box->AddChildToVerticalBox(Tabs);
	UVerticalBox* Graphics = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBox* Audio = WidgetTree->ConstructWidget<UVerticalBox>();
	UVerticalBox* Controls = WidgetTree->ConstructWidget<UVerticalBox>();
	Tabs->AddChild(Graphics);
	Tabs->AddChild(Audio);
	Tabs->AddChild(Controls);

	AddSlider(Graphics, EMetaSettingKey::Gamma, TEXT("SET_GAMMA"), 0.f, 1.f, 0.01f);
	AddCombo(Graphics, EMetaSettingKey::MeshQuality, TEXT("SET_MESH_QUALITY"),
		{ MetaText::Get(TEXT("SET_MQ_0")), MetaText::Get(TEXT("SET_MQ_1")), MetaText::Get(TEXT("SET_MQ_2")), MetaText::Get(TEXT("SET_MQ_3")) });
	AddCombo(Graphics, EMetaSettingKey::AspectRatio, TEXT("SET_ASPECT"), { FText::FromString(TEXT("16:9")), FText::FromString(TEXT("4:3")) });
	AddToggle(Graphics, EMetaSettingKey::MotionBlur, TEXT("SET_MOTION_BLUR"));
	AddSlider(Audio, EMetaSettingKey::MasterVolume, TEXT("SET_MASTER_VOLUME"), 0.f, 1.f, 0.01f);
	AddSlider(Audio, EMetaSettingKey::StereoBalance, TEXT("SET_STEREO"), -1.f, 1.f, 0.01f);
	AddSlider(Controls, EMetaSettingKey::MouseSensitivity, TEXT("SET_MOUSE_SENS"), 0.1f, 3.f, 0.1f);
	AddToggle(Controls, EMetaSettingKey::InvertY, TEXT("SET_INVERT_Y"));

	return Super::RebuildWidget();
}

UMetaSettingsWidget::FRow& UMetaSettingsWidget::AddRow(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, UWidget* Control)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor::Transparent);
	Border->SetPadding(FMargin(8.f, 6.f));
	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>();
	Border->SetContent(H);

	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>();
	LabelBox->SetWidthOverride(LabelWidth);
	LabelBox->AddChild(MetaUI::Text(WidgetTree, MetaText::Get(LabelKey), 18));
	H->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);

	USizeBox* ControlBox = WidgetTree->ConstructWidget<USizeBox>();
	ControlBox->SetWidthOverride(ControlWidth);
	ControlBox->AddChild(Control);
	H->AddChildToHorizontalBox(ControlBox)->SetVerticalAlignment(VAlign_Center);

	UTextBlock* ValueText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 16, FLinearColor(0.8f, 0.8f, 0.8f), TEXT("Mono"));
	UHorizontalBoxSlot* ValueSlot = H->AddChildToHorizontalBox(ValueText);
	ValueSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	ValueSlot->SetVerticalAlignment(VAlign_Center);

	Page->AddChildToVerticalBox(Border);
	FRow& Row = Rows.AddDefaulted_GetRef();
	Row.Key = Key;
	Row.Border = Border;
	Row.ValueText = ValueText;
	return Row;
}

void UMetaSettingsWidget::AddSlider(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, float Min, float Max, float Step)
{
	USlider* S = WidgetTree->ConstructWidget<USlider>();
	S->SetMinValue(Min);
	S->SetMaxValue(Max);
	S->SetStepSize(Step);
	S->OnValueChanged.AddDynamic(this, &UMetaSettingsWidget::HandleSlider);
	AddRow(Page, Key, LabelKey, S).Slider = S;
}

void UMetaSettingsWidget::AddCombo(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey, const TArray<FText>& Options)
{
	UComboBoxString* C = WidgetTree->ConstructWidget<UComboBoxString>();
	for (const FText& O : Options)
	{
		C->AddOption(O.ToString());
	}
	C->OnSelectionChanged.AddDynamic(this, &UMetaSettingsWidget::HandleCombo);
	AddRow(Page, Key, LabelKey, C).Combo = C;
}

void UMetaSettingsWidget::AddToggle(UVerticalBox* Page, EMetaSettingKey Key, const TCHAR* LabelKey)
{
	UCheckBox* C = WidgetTree->ConstructWidget<UCheckBox>();
	C->OnCheckStateChanged.AddDynamic(this, &UMetaSettingsWidget::HandleCheck);
	AddRow(Page, Key, LabelKey, C).Check = C;
}

void UMetaSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UMetaSettingsSubsystem* S = Settings())
	{
		for (FRow& Row : Rows)
		{
			SyncRow(Row, S->GetValue(Row.Key));
		}
		SettingHandle = S->OnSettingChangedNative.AddUObject(this, &UMetaSettingsWidget::HandleSettingChanged);
	}
	RefreshBlinkKeys();
}

void UMetaSettingsWidget::NativeDestruct()
{
	if (UMetaSettingsSubsystem* S = Settings())
	{
		S->OnSettingChangedNative.Remove(SettingHandle);
	}
	Super::NativeDestruct();
}

void UMetaSettingsWidget::RefreshBlinkKeys()
{
	BlinkKeys.Reset();
	const UEncounterSubsystem* Encounters = GetWorld() ? GetWorld()->GetSubsystem<UEncounterSubsystem>() : nullptr;
	if (!Encounters)
	{
		return;
	}
	for (const AMetaBossBase* Boss : Encounters->GetActiveBosses())
	{
		if (Boss->GetGimmick()->GetHintTier() < BlinkTier)
		{
			continue;
		}
		for (const UGimmickCondition* C : Boss->GetGimmick()->Conditions)
		{
			if (const UGimmickCondition_SettingThreshold* T = Cast<UGimmickCondition_SettingThreshold>(C))
			{
				BlinkKeys.AddUnique(T->Key);
			}
		}
	}
}

void UMetaSettingsWidget::HandleSettingChanged(EMetaSettingKey Key, float Old, float New)
{
	for (FRow& Row : Rows)
	{
		if (Row.Key == Key)
		{
			SyncRow(Row, New);
		}
	}
}

void UMetaSettingsWidget::SyncRow(FRow& Row, float Value)
{
	TGuardValue<bool> Guard(bSyncing, true);
	if (Row.Slider) { Row.Slider->SetValue(Value); }
	if (Row.Combo) { Row.Combo->SetSelectedIndex(FMath::RoundToInt(Value)); }
	if (Row.Check) { Row.Check->SetIsChecked(Value > 0.5f); }

	FText Display;
	switch (Row.Key)
	{
	case EMetaSettingKey::Gamma:
	case EMetaSettingKey::MasterVolume:
		Display = FText::AsPercent(Value);
		break;
	case EMetaSettingKey::StereoBalance:
	{
		const int32 Pct = FMath::RoundToInt(FMath::Abs(Value) * 100.f);
		Display = FText::FromString(Pct == 0 ? TEXT("C") : FString::Printf(TEXT("%s%d"), Value < 0.f ? TEXT("L") : TEXT("R"), Pct));
		break;
	}
	case EMetaSettingKey::MouseSensitivity:
		Display = FText::AsNumber(Value, &FNumberFormattingOptions().SetMinimumFractionalDigits(1).SetMaximumFractionalDigits(1));
		break;
	default:
		break;
	}
	Row.ValueText->SetText(Display);
}

float UMetaSettingsWidget::ReadControl(const FRow& Row) const
{
	if (Row.Slider)
	{
		// Snap to the step (1 % for gamma, spec 6.1).
		const float Step = Row.Slider->GetStepSize();
		return Step > 0.f ? FMath::GridSnap(Row.Slider->GetValue(), Step) : Row.Slider->GetValue();
	}
	if (Row.Combo) { return static_cast<float>(FMath::Max(0, Row.Combo->GetSelectedIndex())); }
	return Row.Check && Row.Check->IsChecked() ? 1.f : 0.f;
}

void UMetaSettingsWidget::PushControlsToSettings()
{
	UMetaSettingsSubsystem* S = Settings();
	if (bSyncing || !S)
	{
		return;
	}
	for (const FRow& Row : Rows)
	{
		const float Value = ReadControl(Row);
		if (!FMath::IsNearlyEqual(Value, S->GetValue(Row.Key), KINDA_SMALL_NUMBER))
		{
			S->SetValue(Row.Key, Value); // broadcasts → HandleSettingChanged refreshes the row text
		}
	}
}

void UMetaSettingsWidget::ShowTab0() { Tabs->SetActiveWidgetIndex(0); }
void UMetaSettingsWidget::ShowTab1() { Tabs->SetActiveWidgetIndex(1); }
void UMetaSettingsWidget::ShowTab2() { Tabs->SetActiveWidgetIndex(2); }

void UMetaSettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (BlinkKeys.IsEmpty())
	{
		return;
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(static_cast<float>(MetaUI::Now() * BlinkHz * UE_TWO_PI));
	for (FRow& Row : Rows)
	{
		if (BlinkKeys.Contains(Row.Key))
		{
			Row.Border->SetBrushColor(FLinearColor(MetaUI::Error.R, MetaUI::Error.G, MetaUI::Error.B, 0.15f + 0.45f * Pulse));
		}
	}
}
