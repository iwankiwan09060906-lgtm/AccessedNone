#include "UI/MetaYouDiedWidget.h"
#include "UI/MetaUI.h"
#include "Core/MetaGameMode.h"
#include "Core/MetaLevels.h"
#include "Core/MetaText.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr double ShadeSeconds = 1.2;
	constexpr double BandSeconds = 1.0;
	constexpr double TitleStart = 0.3;
	constexpr double TitleSeconds = 1.8;
	constexpr double ButtonsStart = 1.6;
	constexpr double ButtonsSeconds = 0.6;
	constexpr float ShadeAlpha = 0.12f;  // "겉은 거의 투명하게": the world stays visible
	constexpr float BandAlpha = 0.6f;
	constexpr float BandCore = 120.f;
	constexpr int32 BandFadeSteps = 8;   // stacked strips fake a soft vertical gradient
	constexpr float BandFadeStep = 10.f;
	constexpr int32 TitleSize = 84;
	constexpr int32 TitleSpacing = 600;
	const FLinearColor TitleColor(0.62f, 0.05f, 0.04f);
}

UButton* UMetaYouDiedWidget::MakeButton(const TCHAR* Key)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.75f));
	UTextBlock* Label = MetaUI::Text(WidgetTree, MetaText::Get(Key), 20, FLinearColor(0.92f, 0.88f, 0.8f));
	Label->SetJustification(ETextJustify::Center);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetMinDesiredWidth(200.f);
	Size->AddChild(Label);
	B->SetContent(Size);
	return B;
}

TSharedRef<SWidget> UMetaYouDiedWidget::RebuildWidget()
{
	if (!MetaUI::NeedsBuild(WidgetTree))
	{
		return Super::RebuildWidget();
	}
	SetIsFocusable(true);

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	Shade = WidgetTree->ConstructWidget<UBorder>();
	Shade->SetBrushColor(FLinearColor::Transparent);
	Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Shade);
	ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
	ShadeSlot->SetVerticalAlignment(VAlign_Fill);

	// Band: dark core with strips fading out above and below.
	UVerticalBox* Band = WidgetTree->ConstructWidget<UVerticalBox>();
	Band->SetVisibility(ESlateVisibility::HitTestInvisible);
	auto Strip = [this, Band](float Height, float Alpha)
	{
		USizeBox* S = WidgetTree->ConstructWidget<USizeBox>();
		S->SetHeightOverride(Height);
		UBorder* B = WidgetTree->ConstructWidget<UBorder>();
		B->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, Alpha));
		S->AddChild(B);
		Band->AddChildToVerticalBox(S);
	};
	for (int32 i = 1; i <= BandFadeSteps; ++i) { Strip(BandFadeStep, BandAlpha * i / (BandFadeSteps + 1)); }
	Strip(BandCore, BandAlpha);
	for (int32 i = BandFadeSteps; i >= 1; --i) { Strip(BandFadeStep, BandAlpha * i / (BandFadeSteps + 1)); }
	BandStack = Band;
	UOverlaySlot* BandSlot = Root->AddChildToOverlay(Band);
	BandSlot->SetHorizontalAlignment(HAlign_Fill);
	BandSlot->SetVerticalAlignment(VAlign_Center);

	Title = MetaUI::Text(WidgetTree, MetaText::Get(TEXT("UI_YOU_DIED")), TitleSize, TitleColor);
	Title->SetFont(MetaUI::Font(TitleSize, TEXT("Regular"), TitleSpacing));
	Title->SetShadowOffset(FVector2D(0.f, 3.f));
	Title->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Title->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Title->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* TitleSlot = Root->AddChildToOverlay(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	UButton* Retry = MakeButton(TEXT("UI_RETRY"));
	Retry->OnClicked.AddDynamic(this, &UMetaYouDiedWidget::HandleRetry);
	Row->AddChildToHorizontalBox(Retry)->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
	UButton* Menu = MakeButton(TEXT("UI_TO_MAIN_MENU"));
	Menu->OnClicked.AddDynamic(this, &UMetaYouDiedWidget::HandleMainMenu);
	Row->AddChildToHorizontalBox(Menu);
	Buttons = Row;
	UOverlaySlot* ButtonSlot = Root->AddChildToOverlay(Row);
	ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	ButtonSlot->SetVerticalAlignment(VAlign_Bottom);
	ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 120.f));

	return Super::RebuildWidget();
}

void UMetaYouDiedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StartTime = MetaUI::Now();
	Buttons->SetVisibility(ESlateVisibility::Hidden); // no accidental clicks while the title fades in
}

void UMetaYouDiedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double T = MetaUI::Now() - StartTime;
	auto Ease = [](double X) { X = FMath::Clamp(X, 0.0, 1.0); return static_cast<float>(1.0 - FMath::Pow(1.0 - X, 3.0)); };

	Shade->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, ShadeAlpha * Ease(T / ShadeSeconds)));
	BandStack->SetRenderOpacity(Ease(T / BandSeconds));
	const float A = Ease((T - TitleStart) / TitleSeconds);
	Title->SetRenderOpacity(A);
	Title->SetRenderScale(FVector2D(FMath::Lerp(1.06f, 1.f, A)));

	const float B = Ease((T - ButtonsStart) / ButtonsSeconds);
	if (B > 0.f && Buttons->GetVisibility() == ESlateVisibility::Hidden)
	{
		Buttons->SetVisibility(ESlateVisibility::Visible);
	}
	Buttons->SetRenderOpacity(B);
}

void UMetaYouDiedWidget::HandleRetry()
{
	if (AMetaGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AMetaGameMode>() : nullptr)
	{
		GM->RequestRespawn(); // removes this widget
	}
}

void UMetaYouDiedWidget::HandleMainMenu()
{
	UGameplayStatics::OpenLevel(this, MetaLevels::Title);
}
