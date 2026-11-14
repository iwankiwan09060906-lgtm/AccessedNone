#include "UI/MetaTitleWidget.h"
#include "UI/MetaUI.h"
#include "Core/MetaLevels.h"
#include "Core/MetaLog.h"
#include "Core/MetaSaveGame.h"
#include "Core/MetaText.h"
#include "Meta/HintSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Styling/SlateTypes.h"

TSharedRef<SWidget> UMetaTitleWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		// The 3D title scene shows through; a vignette darkens the edges.
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		UImage* Vignette = WidgetTree->ConstructWidget<UImage>();
		Vignette->SetBrush(MetaUI::TextureBrush(TEXT("T_UI_Vignette"), FLinearColor(0.f, 0.f, 0.01f, 0.95f)));
		UOverlaySlot* VSlot = Root->AddChildToOverlay(Vignette);
		VSlot->SetHorizontalAlignment(HAlign_Fill);
		VSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box);
		BoxSlot->SetHorizontalAlignment(HAlign_Center);
		BoxSlot->SetVerticalAlignment(VAlign_Top);
		BoxSlot->SetPadding(FMargin(0.f, 110.f, 0.f, 0.f));
		// Menu sits low on a dark band so it never fights the lit scene.
		UBorder* MenuBand = WidgetTree->ConstructWidget<UBorder>();
		MenuBand->SetBrush(MetaUI::TextureBrush(TEXT("T_UI_GradientCenter"), FLinearColor(0.f, 0.f, 0.02f, 0.85f)));
		MenuBand->SetPadding(FMargin(260.f, 18.f));
		UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>();
		MenuBand->SetContent(Menu);
		UOverlaySlot* MenuSlot = Root->AddChildToOverlay(MenuBand);
		MenuSlot->SetHorizontalAlignment(HAlign_Center);
		MenuSlot->SetVerticalAlignment(VAlign_Bottom);
		MenuSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 90.f));

		UTextBlock* Title = MetaUI::Text(WidgetTree, MetaText::Get(TEXT("TITLE_GAME")), 80, MetaUI::Gold);
		Title->SetFont(MetaUI::Font(80, TEXT("Bold"), 420));
		Title->SetJustification(ETextJustify::Center);
		Title->SetShadowOffset(FVector2D(0.f, 3.f));
		Title->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Box->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

		// Ornament: ——— ◆ ———
		UHorizontalBox* Ornament = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto Line = [this, Ornament]()
		{
			USizeBox* L = WidgetTree->ConstructWidget<USizeBox>();
			L->SetWidthOverride(180.f);
			L->SetHeightOverride(1.f);
			UBorder* B = WidgetTree->ConstructWidget<UBorder>();
			B->SetBrushColor(FLinearColor(MetaUI::Gold.R, MetaUI::Gold.G, MetaUI::Gold.B, 0.6f));
			L->AddChild(B);
			Ornament->AddChildToHorizontalBox(L)->SetVerticalAlignment(VAlign_Center);
		};
		Line();
		Ornament->AddChildToHorizontalBox(MetaUI::Text(WidgetTree, FText::FromString(TEXT("◆")), 12, MetaUI::Gold))->SetPadding(FMargin(14.f, 0.f));
		Line();
		Box->AddChildToVerticalBox(Ornament)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Cast<UVerticalBoxSlot>(Ornament->Slot)->SetHorizontalAlignment(HAlign_Center);

		AddButton(Menu, TEXT("TITLE_NEW_GAME"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::NewGame);
		if (UGameplayStatics::DoesSaveGameExist(UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex))
		{
			AddButton(Menu, TEXT("TITLE_CONTINUE"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::Continue);
		}
		AddButton(Menu, TEXT("TITLE_QUIT"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::Quit);
	}
	return Super::RebuildWidget();
}

UButton* UMetaTitleWidget::AddButton(UVerticalBox* Box, const TCHAR* Key)
{
	// Text-only entry; hovering lights a soft gold band behind it.
	FButtonStyle Style;
	Style.SetNormal(FSlateNoResource());
	Style.SetHovered(MetaUI::TextureBrush(TEXT("T_UI_GradientCenter"), FLinearColor(MetaUI::Gold.R, MetaUI::Gold.G, MetaUI::Gold.B, 0.35f)));
	Style.SetPressed(MetaUI::TextureBrush(TEXT("T_UI_GradientCenter"), FLinearColor(MetaUI::Gold.R, MetaUI::Gold.G, MetaUI::Gold.B, 0.55f)));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetStyle(Style);
	UTextBlock* Label = MetaUI::Text(WidgetTree, MetaText::Get(Key), 22, FLinearColor(0.9f, 0.86f, 0.76f));
	Label->SetFont(MetaUI::Font(22, TEXT("Regular"), 260));
	Label->SetJustification(ETextJustify::Center);
	Label->SetShadowOffset(FVector2D(0.f, 2.f));
	B->SetContent(Label);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(420.f);
	Size->SetHeightOverride(50.f);
	Size->AddChild(B);
	Box->AddChildToVerticalBox(Size)->SetHorizontalAlignment(HAlign_Center);
	return B;
}

void UMetaTitleWidget::NewGame()
{
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->ResetAllToDefaults();
	GetGameInstance()->GetSubsystem<UHintSubsystem>()->SetDeathCounts({}, {});
	UGameplayStatics::OpenLevel(this, MetaLevels::Prologue);
}

void UMetaTitleWidget::Continue()
{
	const UMetaSaveGame* Save = Cast<UMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex));
	if (!Save)
	{
		UE_LOG(LogAccessedNone, Warning, TEXT("Continue: save slot unreadable, starting a new game"));
		NewGame();
		return;
	}
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->ReadFromSave(*Save);
	GetGameInstance()->GetSubsystem<UHintSubsystem>()->SetDeathCounts(Save->DeathCountPerEncounter, Save->DefeatedEncounters);
	UGameplayStatics::OpenLevel(this, Save->LastLevel.IsNone() ? MetaLevels::Stage1 : Save->LastLevel);
}

void UMetaTitleWidget::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

AMetaTitleGameMode::AMetaTitleGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AMetaTitlePlayerController::StaticClass();
}

void AMetaTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("TitleCamera")))
		{
			SetViewTarget(*It);
		}
	}
	if (USoundBase* Music = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SW_BGM_Prologue.SW_BGM_Prologue"), nullptr, LOAD_Quiet | LOAD_NoWarn))
	{
		UGameplayStatics::SpawnSound2D(this, Music);
	}
	UMetaTitleWidget* Title = CreateWidget<UMetaTitleWidget>(this, MetaUI::WidgetClass<UMetaTitleWidget>(TEXT("WBP_Title")));
	Title->AddToViewport();
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Title->TakeWidget());
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}
