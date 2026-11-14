#include "UI/MetaDeathScreenWidget.h"
#include "UI/MetaUI.h"
#include "Core/MetaText.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr double ShadeSeconds = 1.5;
	constexpr double BandSeconds = 0.6;
	constexpr double TitleStart = 0.4;
	constexpr double TitleSeconds = 1.4;
	constexpr double SubStart = 1.8;
	constexpr double SubCharSeconds = 0.025;
	constexpr double GlitchPeriod = 1.7;  // one short split burst per period, local to the text (spec 12.3)
	constexpr double GlitchLength = 0.12;
	constexpr float BandHeight = 230.f;
	constexpr float TitleSize = 92.f;
}

TSharedRef<SWidget> UMetaDeathScreenWidget::RebuildWidget()
{
	if (!MetaUI::NeedsBuild(WidgetTree))
	{
		return Super::RebuildWidget();
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	Shade = WidgetTree->ConstructWidget<UBorder>();
	Shade->SetBrushColor(FLinearColor::Black);
	UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Shade);
	ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
	ShadeSlot->SetVerticalAlignment(VAlign_Fill);

	// Crimson band with thin bright edges.
	USizeBox* BandSize = WidgetTree->ConstructWidget<USizeBox>();
	BandSize->SetHeightOverride(BandHeight);
	UVerticalBox* BandBox = WidgetTree->ConstructWidget<UVerticalBox>();
	BandSize->AddChild(BandBox);
	auto Edge = [this, BandBox]()
	{
		USizeBox* Line = WidgetTree->ConstructWidget<USizeBox>();
		Line->SetHeightOverride(2.f);
		UBorder* B = WidgetTree->ConstructWidget<UBorder>();
		B->SetBrushColor(MetaUI::Error);
		Line->AddChild(B);
		BandBox->AddChildToVerticalBox(Line);
	};
	Edge();
	Band = WidgetTree->ConstructWidget<UBorder>();
	Band->SetBrushColor(FLinearColor(0.09f, 0.0f, 0.01f, 0.9f));
	BandBox->AddChildToVerticalBox(Band)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Edge();
	UOverlaySlot* BandSlot = Root->AddChildToOverlay(BandSize);
	BandSlot->SetHorizontalAlignment(HAlign_Fill);
	BandSlot->SetVerticalAlignment(VAlign_Center);

	// Title: two offset color copies behind the main text = chromatic split.
	UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlay* Title = WidgetTree->ConstructWidget<UOverlay>();
	const FText Death = MetaText::Get(TEXT("UI_DEATH"));
	SplitA = MetaUI::Text(WidgetTree, Death, TitleSize, FLinearColor(0.f, 0.9f, 1.f, 0.f), TEXT("Bold"));
	SplitB = MetaUI::Text(WidgetTree, Death, TitleSize, FLinearColor(1.f, 0.f, 0.3f, 0.f), TEXT("Bold"));
	UTextBlock* Main = MetaUI::Text(WidgetTree, Death, TitleSize, MetaUI::Error, TEXT("Bold"));
	Main->SetShadowOffset(FVector2D(0.f, 4.f));
	Main->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	for (UTextBlock* T : { SplitA.Get(), SplitB.Get(), Main })
	{
		T->SetJustification(ETextJustify::Center);
		Title->AddChildToOverlay(T)->SetHorizontalAlignment(HAlign_Center);
	}
	TitleStack = Title;
	Title->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Center->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

	SubFull = MetaText::Get(TEXT("UI_DEATH_SUB")).ToString();
	SubText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 16, FLinearColor(0.85f, 0.45f, 0.45f), TEXT("Mono"));
	UVerticalBoxSlot* SubSlot = Center->AddChildToVerticalBox(SubText);
	SubSlot->SetHorizontalAlignment(HAlign_Center);
	SubSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	UOverlaySlot* CenterSlot = Root->AddChildToOverlay(Center);
	CenterSlot->SetHorizontalAlignment(HAlign_Center);
	CenterSlot->SetVerticalAlignment(VAlign_Center);

	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void UMetaDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StartTime = MetaUI::Now();
}

void UMetaDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double T = MetaUI::Now() - StartTime;
	auto Ease = [](double X) { X = FMath::Clamp(X, 0.0, 1.0); return static_cast<float>(1.0 - FMath::Pow(1.0 - X, 3.0)); };

	Shade->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f * Ease(T / ShadeSeconds)));
	Band->GetParent()->GetParent()->SetRenderScale(FVector2D(1.f, Ease(T / BandSeconds)));

	const float A = Ease((T - TitleStart) / TitleSeconds);
	TitleStack->SetRenderOpacity(A);
	TitleStack->SetRenderScale(FVector2D(FMath::Lerp(1.18f, 1.f, A)));

	const bool bGlitch = A >= 1.f && FMath::Fmod(T, GlitchPeriod) < GlitchLength;
	const float Split = bGlitch ? FMath::FRandRange(4.f, 9.f) : 2.f * A;
	SplitA->SetRenderTranslation(FVector2D(-Split, bGlitch ? FMath::FRandRange(-2.f, 2.f) : 0.f));
	SplitB->SetRenderTranslation(FVector2D(Split, 0.f));
	SplitA->SetColorAndOpacity(FSlateColor(FLinearColor(0.f, 0.9f, 1.f, 0.45f * A)));
	SplitB->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.f, 0.3f, 0.45f * A)));

	const int32 Chars = FMath::Clamp(FMath::FloorToInt32((T - SubStart) / SubCharSeconds), 0, SubFull.Len());
	SubText->SetText(FText::FromString(SubFull.Left(Chars)));
}
