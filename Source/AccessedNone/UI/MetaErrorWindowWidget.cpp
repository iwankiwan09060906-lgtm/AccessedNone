#include "UI/MetaErrorWindowWidget.h"
#include "UI/MetaDraggableNumberWidget.h"
#include "UI/MetaUI.h"
#include "Core/MetaLog.h"
#include "Core/MetaText.h"
#include "Meta/HintSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr double ShakeSeconds = 0.4;
	constexpr double DeniedSeconds = 1.5;
	// Overflow sequence (real seconds after solving).
	constexpr double BarBurstSeconds = 1.0;
	constexpr float BarBurstScale = 14.f;
	constexpr double PopupStart = 0.8;
	constexpr double PopupInterval = 0.1;
	constexpr int32 PopupMax = 28; // 2x the errors in the same 4 s
	constexpr double OverflowTotal = 4.0;

	UCanvasPanelSlot* PlaceCentered(UCanvasPanel* Canvas, UWidget* W, FVector2D Anchor)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetAutoSize(true);
		return S;
	}
}

TSharedRef<SWidget> UMetaErrorWindowWidget::RebuildWidget()
{
	if (!MetaUI::NeedsBuild(WidgetTree))
	{
		return Super::RebuildWidget();
	}

	Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;

	// Full-screen backdrop catches clicks outside the window (click-click mode drop = snap back).
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	UCanvasPanelSlot* BackSlot = Root->AddChildToCanvas(Backdrop);
	BackSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackSlot->SetOffsets(FMargin(0.f));

	Window = WidgetTree->ConstructWidget<UBorder>();
	Window->SetBrushColor(FLinearColor(0.12f, 0.12f, 0.13f, 0.97f));
	Window->SetPadding(FMargin(2.f));
	Window->SetClipping(EWidgetClipping::Inherit);
	PlaceCentered(Root, Window, FVector2D(0.5f, 0.45f));

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(600.f);
	Window->SetContent(Size);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Size->AddChild(Box);

	UBorder* TitleBar = WidgetTree->ConstructWidget<UBorder>();
	TitleBar->SetBrushColor(FLinearColor(0.45f, 0.03f, 0.05f));
	TitleBar->SetPadding(FMargin(12.f, 6.f));
	TitleBar->SetContent(MetaUI::Text(WidgetTree, MetaText::Get(TEXT("OP_P5_ERR_TITLE")), 18));
	Box->AddChildToVerticalBox(TitleBar);

	auto AddRow = [this, Box](UWidget* W, float Top)
	{
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(W);
		S->SetPadding(FMargin(24.f, Top, 24.f, 0.f));
		S->SetHorizontalAlignment(HAlign_Center);
		return S;
	};

	AddRow(MetaUI::Text(WidgetTree, MetaText::Get(TEXT("OP_P5_ERR_BODY")), 18), 18.f)->SetHorizontalAlignment(HAlign_Left);

	UHorizontalBox* Fraction = WidgetTree->ConstructWidget<UHorizontalBox>();
	NumeratorWidget = CreateWidget<UMetaDraggableNumberWidget>(this, MetaUI::WidgetClass<UMetaDraggableNumberWidget>(TEXT("WBP_DraggableNumber")));
	NumeratorWidget->Init(this, EMetaNumberSlot::Numerator);
	DenominatorWidget = CreateWidget<UMetaDraggableNumberWidget>(this, MetaUI::WidgetClass<UMetaDraggableNumberWidget>(TEXT("WBP_DraggableNumber")));
	DenominatorWidget->Init(this, EMetaNumberSlot::Denominator);
	Fraction->AddChildToHorizontalBox(NumeratorWidget);
	UHorizontalBoxSlot* SlashSlot = Fraction->AddChildToHorizontalBox(MetaUI::Text(WidgetTree, FText::FromString(TEXT("/")), 30, FLinearColor::White, TEXT("Mono")));
	SlashSlot->SetPadding(FMargin(16.f, 0.f));
	SlashSlot->SetVerticalAlignment(VAlign_Center);
	Fraction->AddChildToHorizontalBox(DenominatorWidget);
	AddRow(Fraction, 18.f);

	UHorizontalBox* ProgressRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
	BarSize->SetWidthOverride(360.f);
	BarSize->SetHeightOverride(16.f);
	Progress = WidgetTree->ConstructWidget<UProgressBar>();
	Progress->SetFillColorAndOpacity(FLinearColor(0.2f, 0.6f, 0.25f));
	BarSize->AddChild(Progress);
	BarSize->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	ProgressRow->AddChildToHorizontalBox(BarSize)->SetVerticalAlignment(VAlign_Center);
	PercentText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 16, FLinearColor::White, TEXT("Mono"));
	ProgressRow->AddChildToHorizontalBox(PercentText)->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	AddRow(ProgressRow, 12.f);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
	for (const TCHAR* Key : { TEXT("OP_P5_BTN_RETRY"), TEXT("OP_P5_BTN_TITLE"), TEXT("OP_P5_BTN_OK") })
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>();
		B->SetContent(MetaUI::Text(WidgetTree, MetaText::Get(Key), 16, FLinearColor::Black));
		B->SetToolTipText(MetaText::Get(TEXT("UI_NO_PERMISSION")));
		B->OnClicked.AddDynamic(this, &UMetaErrorWindowWidget::HandleDeniedButton);
		Buttons->AddChildToHorizontalBox(B)->SetPadding(FMargin(8.f, 0.f));
	}
	AddRow(Buttons, 22.f);

	DeniedText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 14, MetaUI::Error);
	AddRow(DeniedText, 6.f)->SetPadding(FMargin(24.f, 6.f, 24.f, 14.f));

	HeldVisual = MetaUI::Text(WidgetTree, FText::GetEmpty(), 30, MetaUI::Gold, TEXT("Mono"));
	HeldVisual->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* HeldSlot = Root->AddChildToCanvas(HeldVisual);
	HeldSlot->SetAutoSize(true);
	HeldSlot->SetAlignment(FVector2D(0.5f, 0.5f));

	return Super::RebuildWidget();
}

void UMetaErrorWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OpenTime = LastTick = MetaUI::Now();
	RefreshNumbers();
}

void UMetaErrorWindowWidget::RefreshNumbers()
{
	if (!NumeratorWidget)
	{
		return;
	}
	NumeratorWidget->SetValue(State.Get(EMetaNumberSlot::Numerator));
	DenominatorWidget->SetValue(State.Get(EMetaNumberSlot::Denominator));
	const double Ratio = State.Ratio();
	Progress->SetPercent(static_cast<float>(FMath::Min(Ratio, 1.0)));
	FNumberFormattingOptions Opt;
	Opt.MinimumFractionalDigits = State.bSolved ? 0 : 7;
	Opt.MaximumFractionalDigits = 7;
	PercentText->SetText(FText::Format(FText::FromString(TEXT("{0}%")), FText::AsNumber(Ratio * 100.0, &Opt)));
}

void UMetaErrorWindowWidget::BeginGrab(EMetaNumberSlot From)
{
	State.BeginGrab(From);
	RefreshNumbers();
	UE_LOG(LogAccessedNone, Log, TEXT("ErrorWindow: grab %d (counter reset)"), static_cast<int32>(From));
}

void UMetaErrorWindowWidget::BeginHold(EMetaNumberSlot From)
{
	BeginGrab(From);
	bHolding = true;
	HeldVisual->SetText(FText::AsNumber(State.Get(From), &FNumberFormattingOptions::DefaultNoGrouping()));
	HeldVisual->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMetaErrorWindowWidget::DropOn(TOptional<EMetaNumberSlot> Target)
{
	bHolding = false;
	if (HeldVisual) { HeldVisual->SetVisibility(ESlateVisibility::Collapsed); }
	const bool bSolvedNow = State.Drop(Target);
	RefreshNumbers();
	UE_LOG(LogAccessedNone, Log, TEXT("ErrorWindow: drop -> %s (%lld / %lld)"), Target.IsSet() ? TEXT("slot") : TEXT("none"),
		State.Get(EMetaNumberSlot::Numerator), State.Get(EMetaNumberSlot::Denominator));
	if (bSolvedNow)
	{
		OverflowStart = NextPopup = MetaUI::Now();
		NextPopup += PopupStart;
		Progress->SetFillColorAndOpacity(MetaUI::Error);
	}
}

FReply UMetaErrorWindowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHolding)
	{
		DropOn({}); // second click landed outside both numbers
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMetaErrorWindowWidget::HandleDeniedButton()
{
	const double Now = MetaUI::Now();
	ShakeEnd = Now + ShakeSeconds;
	DeniedEnd = Now + DeniedSeconds;
	DeniedText->SetText(MetaText::Get(TEXT("UI_NO_PERMISSION")));
}

void UMetaErrorWindowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = MetaUI::Now();
	const double Delta = Now - LastTick;
	LastTick = Now;

	const int64 Before = State.Get(EMetaNumberSlot::Numerator);
	State.Tick(Delta);
	if (State.Get(EMetaNumberSlot::Numerator) != Before)
	{
		RefreshNumbers();
	}

	Window->SetRenderTranslation(Now < ShakeEnd ? FVector2D(FMath::Sin(Now * 90.0) * 10.0, 0.0) : FVector2D::ZeroVector);
	if (DeniedEnd > 0.0 && Now > DeniedEnd)
	{
		DeniedText->SetText(FText::GetEmpty());
		DeniedEnd = 0.0;
	}

	if (bHolding)
	{
		Cast<UCanvasPanelSlot>(HeldVisual->Slot)->SetPosition(UWidgetLayoutLibrary::GetMousePositionOnViewport(this));
	}

	// Stuck hints (spec 4.5): 30 s, 60 s without the answer.
	if (!State.bSolved && HintsSent < 2 && Now - OpenTime >= (HintsSent == 0 ? HintFirstSeconds : HintSecondSeconds))
	{
		GetGameInstance()->GetSubsystem<UHintSubsystem>()->PushDebuggerMessage(MetaText::Get(HintsSent == 0 ? TEXT("OP_P5_DBG_HINT_30") : TEXT("OP_P5_DBG_HINT_60")));
		++HintsSent;
	}

	if (State.bSolved)
	{
		TickOverflow(Now);
	}
}

void UMetaErrorWindowWidget::TickOverflow(double Now)
{
	const double T = Now - OverflowStart;
	// The progress bar bursts out of the window.
	const float Burst = 1.f + (BarBurstScale - 1.f) * static_cast<float>(FMath::Clamp(T / BarBurstSeconds, 0.0, 1.0));
	Progress->GetParent()->SetRenderScale(FVector2D(Burst, 1.f + (Burst - 1.f) * 0.08f));

	if (Now >= NextPopup && PopupCount < PopupMax)
	{
		NextPopup = Now + PopupInterval;
		++PopupCount;
		UBorder* Popup = WidgetTree->ConstructWidget<UBorder>();
		Popup->SetBrushColor(FLinearColor(0.15f, 0.02f, 0.03f, 0.95f));
		Popup->SetPadding(FMargin(18.f, 12.f));
		Popup->SetContent(MetaUI::Text(WidgetTree, MetaText::Get(TEXT("OP_P5_OVERFLOW")), 18, FLinearColor::White));
		Popup->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceCentered(Root, Popup, FVector2D(FMath::FRandRange(0.2f, 0.8f), FMath::FRandRange(0.15f, 0.85f)));
	}

	if (!bFinished && T >= OverflowTotal)
	{
		bFinished = true;
		OnFinished.Broadcast();
	}
}
