#include "UI/MetaSettingsWidget.h"
#include "UI/MetaUI.h"
#include "Characters/MetaBossBase.h"
#include "Core/MetaLevels.h"
#include "Core/MetaPlayerController.h"
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
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	constexpr int32 BlinkTier = 8;      // spec 6.3
	constexpr double BlinkHz = 1.0;     // well under the 3 Hz photosensitivity limit (12.3)
	constexpr float LabelWidth = 300.f;
	constexpr float ControlWidth = 300.f;
	constexpr float PageHeight = 480.f; // pages scroll
	const FLinearColor TabIdle(0.55f, 0.55f, 0.58f);

	TArray<FText> Opts(std::initializer_list<const TCHAR*> Keys)
	{
		TArray<FText> Out;
		for (const TCHAR* K : Keys) { Out.Add(MetaText::Get(K)); }
		return Out;
	}
	/** Option labels that are not words (resolutions, "×16", "FXAA"): not translatable text. */
	TArray<FText> Literal(std::initializer_list<const TCHAR*> Items)
	{
		TArray<FText> Out;
		for (const TCHAR* S : Items) { Out.Add(FText::FromString(S)); }
		return Out;
	}
	TArray<FText> Quality4() { return Opts({ TEXT("SET_OPT_Q_LOW"), TEXT("SET_OPT_Q_MED"), TEXT("SET_OPT_Q_HIGH"), TEXT("SET_OPT_Q_EPIC") }); }
	TArray<FText> Concat(TArray<FText> A, const TArray<FText>& B) { A.Append(B); return A; }
}

UMetaSettingsSubsystem* UMetaSettingsWidget::Settings() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>() : nullptr;
}

UButton* UMetaSettingsWidget::MakeButton(const FText& Label, int32 Size)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetContent(MetaUI::Text(WidgetTree, Label, Size, FLinearColor::Black));
	return B;
}

TSharedRef<SWidget> UMetaSettingsWidget::RebuildWidget()
{
	if (!MetaUI::NeedsBuild(WidgetTree))
	{
		return Super::RebuildWidget();
	}
	SetIsFocusable(true); // key capture for the binding page

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

	TabBar = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(TabBar)->SetPadding(FMargin(0.f, 14.f));
	Tabs = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Box->AddChildToVerticalBox(Tabs);

	// ---------------------------------------------------------------- 게임플레이
	UVerticalBox* P = AddPage(TEXT("SET_TAB_GAMEPLAY"));
	AddCombo(P, Decoy(TEXT("SET_LANGUAGE"), 0.f), TEXT("SET_LANGUAGE"), Opts({ TEXT("SET_OPT_LANG_KO"), TEXT("SET_OPT_LANG_EN"), TEXT("SET_OPT_LANG_JA") }));
	AddCombo(P, Decoy(TEXT("SET_DIFFICULTY"), 1.f), TEXT("SET_DIFFICULTY"), Opts({ TEXT("SET_OPT_EASY"), TEXT("SET_OPT_Q_MED"), TEXT("SET_OPT_HARD"), TEXT("SET_OPT_NIGHTMARE") }));
	AddToggle(P, Decoy(TEXT("SET_DAMAGE_NUMBERS"), 1.f), TEXT("SET_DAMAGE_NUMBERS"));
	AddCombo(P, Decoy(TEXT("SET_HUD_MODE"), 0.f), TEXT("SET_HUD_MODE"), Opts({ TEXT("SET_OPT_ALWAYS"), TEXT("SET_OPT_IN_COMBAT"), TEXT("SET_OPT_HIDDEN") }));
	AddSlider(P, Decoy(TEXT("SET_CAMERA_SHAKE"), 1.f), TEXT("SET_CAMERA_SHAKE"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_CAMERA_DISTANCE"), 1.f), TEXT("SET_CAMERA_DISTANCE"), 0.5f, 1.5f, 0.05f, EFormat::Percent);
	AddToggle(P, Decoy(TEXT("SET_TUTORIAL_TIPS"), 1.f), TEXT("SET_TUTORIAL_TIPS"));
	AddToggle(P, Decoy(TEXT("SET_AUTOSAVE_NOTICE"), 1.f), TEXT("SET_AUTOSAVE_NOTICE"));
	AddToggle(P, Decoy(TEXT("SET_SUBTITLES"), 1.f), TEXT("SET_SUBTITLES"));
	AddCombo(P, Decoy(TEXT("SET_BLOOD"), 2.f), TEXT("SET_BLOOD"), Opts({ TEXT("SET_OPT_OFF"), TEXT("SET_OPT_Q_LOW"), TEXT("SET_OPT_Q_HIGH") }));

	// ---------------------------------------------------------------- 그래픽 (E1 gamma, E5 aspect)
	P = AddPage(TEXT("SET_TAB_GRAPHICS"));
	AddCombo(P, Decoy(TEXT("SET_WINDOW_MODE"), 0.f), TEXT("SET_WINDOW_MODE"), Opts({ TEXT("SET_OPT_FULLSCREEN"), TEXT("SET_OPT_BORDERLESS"), TEXT("SET_OPT_WINDOWED") }));
	AddCombo(P, Decoy(TEXT("SET_RESOLUTION"), 2.f), TEXT("SET_RESOLUTION"), Literal({ TEXT("1280 × 720"), TEXT("1600 × 900"), TEXT("1920 × 1080"), TEXT("2560 × 1440"), TEXT("3840 × 2160") }));
	AddCombo(P, Real(EMetaSettingKey::AspectRatio), TEXT("SET_ASPECT"), Literal({ TEXT("16:9"), TEXT("4:3") }));
	AddToggle(P, Decoy(TEXT("SET_VSYNC"), 1.f), TEXT("SET_VSYNC"));
	AddCombo(P, Decoy(TEXT("SET_FRAME_LIMIT"), 1.f), TEXT("SET_FRAME_LIMIT"), Concat(Literal({ TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144") }), Opts({ TEXT("SET_OPT_UNLIMITED") })));
	AddSlider(P, Decoy(TEXT("SET_RENDER_SCALE"), 1.f), TEXT("SET_RENDER_SCALE"), 0.5f, 2.f, 0.05f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_FOV"), 90.f), TEXT("SET_FOV"), 60.f, 120.f, 1.f, EFormat::Integer);
	AddSlider(P, Real(EMetaSettingKey::Gamma), TEXT("SET_GAMMA"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_CONTRAST"), 0.5f), TEXT("SET_CONTRAST"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_SATURATION"), 0.5f), TEXT("SET_SATURATION"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddToggle(P, Decoy(TEXT("SET_HDR"), 0.f), TEXT("SET_HDR"));
	AddToggle(P, Real(EMetaSettingKey::MotionBlur), TEXT("SET_MOTION_BLUR"));
	AddToggle(P, Decoy(TEXT("SET_FILM_GRAIN"), 0.f), TEXT("SET_FILM_GRAIN"));
	AddToggle(P, Decoy(TEXT("SET_CHROMATIC"), 0.f), TEXT("SET_CHROMATIC"));
	AddToggle(P, Decoy(TEXT("SET_VIGNETTE"), 1.f), TEXT("SET_VIGNETTE"));
	AddToggle(P, Decoy(TEXT("SET_DOF"), 1.f), TEXT("SET_DOF"));
	AddToggle(P, Decoy(TEXT("SET_BLOOM"), 1.f), TEXT("SET_BLOOM"));
	AddToggle(P, Decoy(TEXT("SET_LENS_FLARE"), 1.f), TEXT("SET_LENS_FLARE"));

	// ---------------------------------------------------------------- 품질 (E2 mesh quality, among look-alikes)
	P = AddPage(TEXT("SET_TAB_QUALITY"));
	AddCombo(P, Decoy(TEXT("SET_PRESET"), 3.f), TEXT("SET_PRESET"), Concat(Quality4(), Opts({ TEXT("SET_OPT_CUSTOM") })));
	AddCombo(P, Decoy(TEXT("SET_TEXTURE_QUALITY"), 3.f), TEXT("SET_TEXTURE_QUALITY"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_SHADOW_QUALITY"), 3.f), TEXT("SET_SHADOW_QUALITY"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_CHARACTER_DETAIL"), 3.f), TEXT("SET_CHARACTER_DETAIL"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_VIEW_DISTANCE"), 2.f), TEXT("SET_VIEW_DISTANCE"), Opts({ TEXT("SET_OPT_NEAR"), TEXT("SET_OPT_Q_MED"), TEXT("SET_OPT_FAR"), TEXT("SET_OPT_MAX") }));
	AddCombo(P, Decoy(TEXT("SET_EFFECTS_QUALITY"), 3.f), TEXT("SET_EFFECTS_QUALITY"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_POSTPROCESS_QUALITY"), 3.f), TEXT("SET_POSTPROCESS_QUALITY"), Quality4());
	AddCombo(P, Real(EMetaSettingKey::MeshQuality), TEXT("SET_MESH_QUALITY"),
		Opts({ TEXT("SET_MQ_0"), TEXT("SET_MQ_1"), TEXT("SET_MQ_2"), TEXT("SET_MQ_3") }));
	AddCombo(P, Decoy(TEXT("SET_FOLIAGE_QUALITY"), 3.f), TEXT("SET_FOLIAGE_QUALITY"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_LOD_DISTANCE"), 3.f), TEXT("SET_LOD_DISTANCE"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_ANTI_ALIASING"), 3.f), TEXT("SET_ANTI_ALIASING"), Concat(Opts({ TEXT("SET_OPT_OFF") }), Literal({ TEXT("FXAA"), TEXT("TAA"), TEXT("TSR") })));
	AddCombo(P, Decoy(TEXT("SET_ANISOTROPIC"), 4.f), TEXT("SET_ANISOTROPIC"), Concat(Opts({ TEXT("SET_OPT_OFF") }), Literal({ TEXT("×2"), TEXT("×4"), TEXT("×8"), TEXT("×16") })));
	AddCombo(P, Decoy(TEXT("SET_GI_QUALITY"), 3.f), TEXT("SET_GI_QUALITY"), Quality4());
	AddCombo(P, Decoy(TEXT("SET_REFLECTION_QUALITY"), 3.f), TEXT("SET_REFLECTION_QUALITY"), Quality4());
	AddToggle(P, Decoy(TEXT("SET_AO"), 1.f), TEXT("SET_AO"));
	AddCombo(P, Decoy(TEXT("SET_SHADING_QUALITY"), 3.f), TEXT("SET_SHADING_QUALITY"), Quality4());
	AddToggle(P, Decoy(TEXT("SET_RAY_TRACING"), 0.f), TEXT("SET_RAY_TRACING"));
	AddCombo(P, Decoy(TEXT("SET_UPSCALER"), 0.f), TEXT("SET_UPSCALER"), Concat(Opts({ TEXT("SET_OPT_OFF") }), Literal({ TEXT("FSR"), TEXT("DLSS"), TEXT("XeSS") })));

	// ---------------------------------------------------------------- 오디오 (E4 stereo balance)
	P = AddPage(TEXT("SET_TAB_AUDIO"));
	AddSlider(P, Real(EMetaSettingKey::MasterVolume), TEXT("SET_MASTER_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_MUSIC_VOLUME"), 0.7f), TEXT("SET_MUSIC_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_SFX_VOLUME"), 0.8f), TEXT("SET_SFX_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_VOICE_VOLUME"), 1.f), TEXT("SET_VOICE_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_AMBIENT_VOLUME"), 0.7f), TEXT("SET_AMBIENT_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_UI_VOLUME"), 0.6f), TEXT("SET_UI_VOLUME"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddCombo(P, Decoy(TEXT("SET_OUTPUT_DEVICE"), 0.f), TEXT("SET_OUTPUT_DEVICE"), Opts({ TEXT("SET_OPT_SYSTEM_DEFAULT"), TEXT("SET_OPT_SPEAKERS"), TEXT("SET_OPT_HEADPHONES") }));
	AddCombo(P, Decoy(TEXT("SET_SPEAKER_CONFIG"), 0.f), TEXT("SET_SPEAKER_CONFIG"), Concat(Opts({ TEXT("SET_OPT_STEREO") }), Literal({ TEXT("5.1"), TEXT("7.1") })));
	AddSlider(P, Real(EMetaSettingKey::StereoBalance), TEXT("SET_STEREO"), -1.f, 1.f, 0.01f, EFormat::Balance);
	AddCombo(P, Decoy(TEXT("SET_DYNAMIC_RANGE"), 0.f), TEXT("SET_DYNAMIC_RANGE"), Opts({ TEXT("SET_OPT_WIDE"), TEXT("SET_OPT_Q_MED"), TEXT("SET_OPT_NIGHT_MODE") }));
	AddToggle(P, Decoy(TEXT("SET_MUTE_BACKGROUND"), 1.f), TEXT("SET_MUTE_BACKGROUND"));
	AddToggle(P, Decoy(TEXT("SET_SPATIAL_AUDIO"), 0.f), TEXT("SET_SPATIAL_AUDIO"));
	AddToggle(P, Decoy(TEXT("SET_LOW_HP_SOUND"), 1.f), TEXT("SET_LOW_HP_SOUND"));

	// ---------------------------------------------------------------- 조작
	P = AddPage(TEXT("SET_TAB_CONTROLS"));
	AddSlider(P, Real(EMetaSettingKey::MouseSensitivity), TEXT("SET_MOUSE_SENS"), 0.1f, 3.f, 0.1f, EFormat::OneDecimal);
	AddSlider(P, Decoy(TEXT("SET_LOCKON_SENS"), 1.f), TEXT("SET_LOCKON_SENS"), 0.1f, 3.f, 0.1f, EFormat::OneDecimal);
	AddToggle(P, Real(EMetaSettingKey::InvertY), TEXT("SET_INVERT_Y"));
	AddToggle(P, Decoy(TEXT("SET_INVERT_X"), 0.f), TEXT("SET_INVERT_X"));
	AddToggle(P, Decoy(TEXT("SET_MOUSE_ACCEL"), 0.f), TEXT("SET_MOUSE_ACCEL"));
	AddSlider(P, Decoy(TEXT("SET_MOUSE_SMOOTHING"), 0.f), TEXT("SET_MOUSE_SMOOTHING"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddCombo(P, Decoy(TEXT("SET_LOCKON_MODE"), 0.f), TEXT("SET_LOCKON_MODE"), Opts({ TEXT("SET_OPT_TOGGLE"), TEXT("SET_OPT_HOLD") }));
	AddToggle(P, Decoy(TEXT("SET_LOCKON_SWITCH"), 1.f), TEXT("SET_LOCKON_SWITCH"));
	AddToggle(P, Decoy(TEXT("SET_CAMERA_AUTO"), 0.f), TEXT("SET_CAMERA_AUTO"));
	AddCombo(P, Decoy(TEXT("SET_DODGE_INPUT"), 0.f), TEXT("SET_DODGE_INPUT"), Opts({ TEXT("SET_OPT_TAP"), TEXT("SET_OPT_DOUBLE_TAP") }));
	AddToggle(P, Decoy(TEXT("SET_VIBRATION"), 1.f), TEXT("SET_VIBRATION"));
	AddSlider(P, Decoy(TEXT("SET_VIBRATION_STRENGTH"), 1.f), TEXT("SET_VIBRATION_STRENGTH"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddSlider(P, Decoy(TEXT("SET_STICK_DEADZONE"), 0.15f), TEXT("SET_STICK_DEADZONE"), 0.f, 0.5f, 0.01f, EFormat::Percent);

	// ---------------------------------------------------------------- 키 설정
	P = AddPage(TEXT("SET_TAB_KEYS"));
	P->AddChildToVerticalBox(MetaUI::Text(WidgetTree, MetaText::Get(TEXT("SET_KEY_HINT")), 14, FLinearColor(0.7f, 0.7f, 0.7f)))->SetPadding(FMargin(8.f, 0.f, 0.f, 8.f));
	AddBinding(P, TEXT("SET_KEY_MOVE_F"), EKeys::W);
	AddBinding(P, TEXT("SET_KEY_MOVE_B"), EKeys::S);
	AddBinding(P, TEXT("SET_KEY_MOVE_L"), EKeys::A);
	AddBinding(P, TEXT("SET_KEY_MOVE_R"), EKeys::D);
	AddBinding(P, TEXT("SET_KEY_DODGE"), EKeys::SpaceBar);
	AddBinding(P, TEXT("SET_KEY_JUMP"), EKeys::F);
	AddBinding(P, TEXT("SET_KEY_LOCKON"), EKeys::MiddleMouseButton);
	AddBinding(P, TEXT("SET_KEY_LIGHT"), EKeys::LeftMouseButton);
	AddBinding(P, TEXT("SET_KEY_HEAVY"), EKeys::LeftShift);
	AddBinding(P, TEXT("SET_KEY_ITEM"), EKeys::R);
	AddBinding(P, TEXT("SET_KEY_INTERACT"), EKeys::E);
	AddBinding(P, TEXT("SET_KEY_QUICKSLOT"), EKeys::Q);
	AddBinding(P, TEXT("SET_KEY_GESTURE"), EKeys::G);
	AddBinding(P, TEXT("SET_KEY_MAP"), EKeys::M);
	AddBinding(P, TEXT("SET_KEY_MENU"), EKeys::Escape);

	// ---------------------------------------------------------------- 접근성
	P = AddPage(TEXT("SET_TAB_ACCESS"));
	AddCombo(P, Decoy(TEXT("SET_COLORBLIND"), 0.f), TEXT("SET_COLORBLIND"), Opts({ TEXT("SET_OPT_OFF"), TEXT("SET_OPT_PROTAN"), TEXT("SET_OPT_DEUTAN"), TEXT("SET_OPT_TRITAN") }));
	AddSlider(P, Decoy(TEXT("SET_COLORBLIND_STRENGTH"), 1.f), TEXT("SET_COLORBLIND_STRENGTH"), 0.f, 1.f, 0.01f, EFormat::Percent);
	AddCombo(P, Decoy(TEXT("SET_SUBTITLE_SIZE"), 1.f), TEXT("SET_SUBTITLE_SIZE"), Opts({ TEXT("SET_OPT_SMALL"), TEXT("SET_OPT_Q_MED"), TEXT("SET_OPT_LARGE"), TEXT("SET_OPT_XLARGE") }));
	AddToggle(P, Decoy(TEXT("SET_SUBTITLE_BG"), 1.f), TEXT("SET_SUBTITLE_BG"));
	AddToggle(P, Decoy(TEXT("SET_SPEAKER_NAMES"), 1.f), TEXT("SET_SPEAKER_NAMES"));
	AddSlider(P, Decoy(TEXT("SET_UI_SCALE"), 1.f), TEXT("SET_UI_SCALE"), 0.8f, 1.5f, 0.05f, EFormat::Percent);
	AddToggle(P, Decoy(TEXT("SET_HIGH_CONTRAST"), 0.f), TEXT("SET_HIGH_CONTRAST"));
	AddToggle(P, Decoy(TEXT("SET_REDUCE_SHAKE"), 0.f), TEXT("SET_REDUCE_SHAKE"));
	AddToggle(P, Decoy(TEXT("SET_REDUCE_FLASH"), 1.f), TEXT("SET_REDUCE_FLASH"));
	AddToggle(P, Decoy(TEXT("SET_MONO_AUDIO"), 0.f), TEXT("SET_MONO_AUDIO"));
	AddToggle(P, Decoy(TEXT("SET_TTS"), 0.f), TEXT("SET_TTS"));
	AddToggle(P, Decoy(TEXT("SET_HOLD_TO_TOGGLE"), 0.f), TEXT("SET_HOLD_TO_TOGGLE"));

	// ---------------------------------------------------------------- footer
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	UButton* Reset = MakeButton(MetaText::Get(TEXT("SET_BTN_RESET")), 16);
	Reset->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleReset);
	Footer->AddChildToHorizontalBox(Reset);
	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>();
	Footer->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* MainMenu = MakeButton(MetaText::Get(TEXT("SET_BTN_MAIN_MENU")), 16);
	MainMenu->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleMainMenu);
	Footer->AddChildToHorizontalBox(MainMenu)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UButton* Quit = MakeButton(MetaText::Get(TEXT("SET_BTN_QUIT")), 16);
	Quit->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleQuit);
	Footer->AddChildToHorizontalBox(Quit)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UButton* Close = MakeButton(MetaText::Get(TEXT("SET_BTN_CLOSE")), 16);
	Close->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleClose);
	Footer->AddChildToHorizontalBox(Close)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));

	ShowTab(1); // graphics first, like most games
	return Super::RebuildWidget();
}

UVerticalBox* UMetaSettingsWidget::AddPage(const TCHAR* TabKey)
{
	UButton* B = MakeButton(MetaText::Get(TabKey), 16);
	B->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleTabClicked);
	TabBar->AddChildToHorizontalBox(B)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	TabButtons.Add(B);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(PageHeight);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Size->AddChild(Scroll);
	UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(Page);
	Tabs->AddChild(Size);
	return Page;
}

UMetaSettingsWidget::FRow& UMetaSettingsWidget::AddRow(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, UWidget* Control)
{
	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetBrushColor(FLinearColor::Transparent);
	Border->SetPadding(FMargin(8.f, 5.f));
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
	ValueSlot->SetPadding(FMargin(16.f, 0.f, 24.f, 0.f));
	ValueSlot->SetVerticalAlignment(VAlign_Center);

	Page->AddChildToVerticalBox(Border);
	FRow& Row = Rows.AddDefaulted_GetRef();
	Row.Ref = Ref;
	Row.Border = Border;
	Row.ValueText = ValueText;
	return Row;
}

void UMetaSettingsWidget::AddSlider(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, float Min, float Max, float Step, EFormat Format)
{
	USlider* S = WidgetTree->ConstructWidget<USlider>();
	S->SetMinValue(Min);
	S->SetMaxValue(Max);
	S->SetStepSize(Step);
	S->OnValueChanged.AddDynamic(this, &UMetaSettingsWidget::HandleSlider);
	FRow& Row = AddRow(Page, Ref, LabelKey, S);
	Row.Slider = S;
	Row.Format = Format;
}

void UMetaSettingsWidget::AddCombo(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey, const TArray<FText>& Options)
{
	UComboBoxString* C = WidgetTree->ConstructWidget<UComboBoxString>();
	for (const FText& O : Options)
	{
		C->AddOption(O.ToString());
	}
	C->OnSelectionChanged.AddDynamic(this, &UMetaSettingsWidget::HandleCombo);
	AddRow(Page, Ref, LabelKey, C).Combo = C;
}

void UMetaSettingsWidget::AddToggle(UVerticalBox* Page, const FRef& Ref, const TCHAR* LabelKey)
{
	UCheckBox* C = WidgetTree->ConstructWidget<UCheckBox>();
	C->OnCheckStateChanged.AddDynamic(this, &UMetaSettingsWidget::HandleCheck);
	AddRow(Page, Ref, LabelKey, C).Check = C;
}

void UMetaSettingsWidget::AddBinding(UVerticalBox* Page, const TCHAR* LabelKey, const FKey& Default)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* KeyText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 16, FLinearColor::Black, TEXT("Bold"));
	B->SetContent(KeyText);
	B->OnClicked.AddDynamic(this, &UMetaSettingsWidget::HandleBindClicked);
	AddRow(Page, Decoy(LabelKey, 0.f), LabelKey, B); // row for layout only: no slider/combo/check, never pushed
	FBindRow& Bind = Bindings.AddDefaulted_GetRef();
	Bind.Id = LabelKey;
	Bind.Default = Default;
	Bind.Button = B;
	Bind.KeyText = KeyText;
}

void UMetaSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UMetaSettingsSubsystem* S = Settings())
	{
		SettingHandle = S->OnSettingChangedNative.AddUObject(this, &UMetaSettingsWidget::HandleSettingChanged);
	}
	SyncAll();
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

void UMetaSettingsWidget::SyncAll()
{
	for (FRow& Row : Rows)
	{
		SyncRow(Row, ReadSetting(Row));
	}
	for (FBindRow& Bind : Bindings)
	{
		RefreshBinding(Bind);
	}
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
		if (Row.Ref.bReal && Row.Ref.Key == Key)
		{
			SyncRow(Row, New);
		}
	}
}

float UMetaSettingsWidget::ReadSetting(const FRow& Row) const
{
	const UMetaSettingsSubsystem* S = Settings();
	if (Row.Ref.bReal)
	{
		return S ? S->GetValue(Row.Ref.Key) : UMetaSettingsSubsystem::GetDefaultValue(Row.Ref.Key);
	}
	const float* Found = S ? S->DecoyValues.Find(Row.Ref.Decoy) : nullptr;
	return Found ? *Found : Row.Ref.Default;
}

void UMetaSettingsWidget::SyncRow(FRow& Row, float Value)
{
	TGuardValue<bool> Guard(bSyncing, true);
	if (Row.Slider) { Row.Slider->SetValue(Value); }
	if (Row.Combo) { Row.Combo->SetSelectedIndex(FMath::RoundToInt(Value)); }
	if (Row.Check) { Row.Check->SetIsChecked(Value > 0.5f); }

	FText Display;
	switch (Row.Format)
	{
	case EFormat::Percent:
		Display = FText::AsPercent(Value);
		break;
	case EFormat::Balance:
	{
		const int32 Pct = FMath::RoundToInt(FMath::Abs(Value) * 100.f);
		Display = FText::FromString(Pct == 0 ? TEXT("C") : FString::Printf(TEXT("%s%d"), Value < 0.f ? TEXT("L") : TEXT("R"), Pct));
		break;
	}
	case EFormat::OneDecimal:
		Display = FText::AsNumber(Value, &FNumberFormattingOptions().SetMinimumFractionalDigits(1).SetMaximumFractionalDigits(1));
		break;
	case EFormat::Integer:
		Display = FText::AsNumber(FMath::RoundToInt(Value));
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
	for (FRow& Row : Rows)
	{
		if (!Row.Slider && !Row.Combo && !Row.Check)
		{
			continue; // key-binding rows
		}
		const float Value = ReadControl(Row);
		if (FMath::IsNearlyEqual(Value, ReadSetting(Row), KINDA_SMALL_NUMBER))
		{
			continue;
		}
		if (Row.Ref.bReal)
		{
			S->SetValue(Row.Ref.Key, Value); // broadcasts → HandleSettingChanged refreshes the row text
		}
		else
		{
			S->DecoyValues.Add(Row.Ref.Decoy, Value);
			SyncRow(Row, Value);
		}
	}
}

void UMetaSettingsWidget::RefreshBinding(FBindRow& Row)
{
	const UMetaSettingsSubsystem* S = Settings();
	const FKey* Found = S ? S->DecoyKeyBinds.Find(Row.Id) : nullptr;
	Row.KeyText->SetText((Found ? *Found : Row.Default).GetDisplayName());
}

FReply UMetaSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!Bindings.IsValidIndex(ListeningBind))
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent); // ESC falls through to IA_Menu and closes the menu
	}
	FBindRow& Bind = Bindings[ListeningBind];
	ListeningBind = INDEX_NONE;
	UMetaSettingsSubsystem* S = Settings();
	if (S && InKeyEvent.GetKey() != EKeys::Escape) // ESC cancels
	{
		// shortcut: cosmetic only, the real Enhanced Input mappings stay as they are; wire EnhancedInputUserSettings to make it real.
		S->DecoyKeyBinds.Add(Bind.Id, InKeyEvent.GetKey());
	}
	RefreshBinding(Bind);
	return FReply::Handled();
}

void UMetaSettingsWidget::HandleBindClicked()
{
	for (int32 i = 0; i < Bindings.Num(); ++i)
	{
		if (Bindings[i].Button->IsHovered() || Bindings[i].Button->HasKeyboardFocus())
		{
			if (Bindings.IsValidIndex(ListeningBind)) { RefreshBinding(Bindings[ListeningBind]); }
			ListeningBind = i;
			Bindings[i].KeyText->SetText(MetaText::Get(TEXT("SET_KEY_PRESS")));
			SetKeyboardFocus(); // the button itself would eat Space/Enter as clicks
			return;
		}
	}
}

void UMetaSettingsWidget::HandleTabClicked()
{
	for (int32 i = 0; i < TabButtons.Num(); ++i)
	{
		if (TabButtons[i]->IsHovered() || TabButtons[i]->HasKeyboardFocus())
		{
			ShowTab(i);
			return;
		}
	}
}

void UMetaSettingsWidget::ShowTab(int32 Index)
{
	Tabs->SetActiveWidgetIndex(Index);
	for (int32 i = 0; i < TabButtons.Num(); ++i)
	{
		TabButtons[i]->SetBackgroundColor(i == Index ? MetaUI::Gold : TabIdle);
	}
}

void UMetaSettingsWidget::HandleReset()
{
	if (UMetaSettingsSubsystem* S = Settings())
	{
		S->DecoyValues.Reset();
		S->DecoyKeyBinds.Reset();
		for (const FRow& Row : Rows)
		{
			if (Row.Ref.bReal) { S->SetValue(Row.Ref.Key, UMetaSettingsSubsystem::GetDefaultValue(Row.Ref.Key)); }
		}
	}
	ListeningBind = INDEX_NONE;
	SyncAll();
}

void UMetaSettingsWidget::HandleMainMenu()
{
	UGameplayStatics::OpenLevel(this, MetaLevels::Title);
}

void UMetaSettingsWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMetaSettingsWidget::HandleClose()
{
	if (AMetaPlayerController* PC = Cast<AMetaPlayerController>(GetOwningPlayer()))
	{
		PC->ToggleMenu();
	}
}

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
		if (Row.Ref.bReal && BlinkKeys.Contains(Row.Ref.Key))
		{
			Row.Border->SetBrushColor(FLinearColor(MetaUI::Error.R, MetaUI::Error.G, MetaUI::Error.B, 0.15f + 0.45f * Pulse));
		}
	}
}
