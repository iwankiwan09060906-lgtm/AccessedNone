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

TSharedRef<SWidget> UMetaTitleWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
		Root->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.025f));
		Root->SetHorizontalAlignment(HAlign_Center);
		Root->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Root;

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Root->SetContent(Box);
		UTextBlock* Title = MetaUI::Text(WidgetTree, MetaText::Get(TEXT("TITLE_GAME")), 72, MetaUI::Gold, TEXT("Bold"));
		Title->SetJustification(ETextJustify::Center);
		Box->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));

		AddButton(Box, TEXT("TITLE_NEW_GAME"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::NewGame);
		if (UGameplayStatics::DoesSaveGameExist(UMetaSaveGame::SlotName, UMetaSaveGame::UserIndex))
		{
			AddButton(Box, TEXT("TITLE_CONTINUE"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::Continue);
		}
		AddButton(Box, TEXT("TITLE_QUIT"))->OnClicked.AddDynamic(this, &UMetaTitleWidget::Quit);
	}
	return Super::RebuildWidget();
}

UButton* UMetaTitleWidget::AddButton(UVerticalBox* Box, const TCHAR* Key)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetBackgroundColor(FLinearColor(0.1f, 0.1f, 0.12f));
	UTextBlock* Label = MetaUI::Text(WidgetTree, MetaText::Get(Key), 24, FLinearColor(0.9f, 0.88f, 0.8f));
	Label->SetJustification(ETextJustify::Center);
	B->SetContent(Label);
	UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B);
	S->SetPadding(FMargin(120.f, 6.f));
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
	UMetaTitleWidget* Title = CreateWidget<UMetaTitleWidget>(this, MetaUI::WidgetClass<UMetaTitleWidget>(TEXT("WBP_Title")));
	Title->AddToViewport();
	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(Title->TakeWidget());
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}
