#include "UI/MetaBossBarOverflowWidget.h"
#include "UI/MetaUI.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr float BarHeight = 14.f;
	constexpr float StartWidthFraction = 0.35f;
	constexpr float Margin = 40.f;
}

TSharedRef<SWidget> UMetaBossBarOverflowWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		Lines = WidgetTree->ConstructWidget<UVerticalBox>();
		Lines->SetRenderTranslation(FVector2D(Margin, 24.f));
		WidgetTree->RootWidget = Lines;
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UMetaBossBarOverflowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StartTime = MetaUI::Now();
	AddLine();
}

UWidget* UMetaBossBarOverflowWidget::AddLine()
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetHeightOverride(BarHeight);
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>();
	Bar->SetBrushColor(FLinearColor(0.6f, 0.03f, 0.04f));
	Size->AddChild(Bar);
	Lines->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.f, 3.f));
	return Size;
}

int32 UMetaBossBarOverflowWidget::GetLineCount() const
{
	return Lines ? Lines->GetChildrenCount() : 0;
}

void UMetaBossBarOverflowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = MetaUI::Now();
	const float FullWidth = MyGeometry.GetLocalSize().X; // overflow: rows run past the right edge by Margin
	USizeBox* First = Cast<USizeBox>(Lines->GetChildAt(0));

	const float Alpha = static_cast<float>(FMath::Clamp((Now - StartTime) / FirstLineSeconds, 0.0, 1.0));
	First->SetWidthOverride(FMath::Lerp(StartWidthFraction, 1.f, Alpha) * FullWidth);
	if (Alpha < 1.f)
	{
		NextLine = Now;
		return;
	}
	for (int32 i = 1; i < Lines->GetChildrenCount(); ++i)
	{
		Cast<USizeBox>(Lines->GetChildAt(i))->SetWidthOverride(FullWidth);
	}
	if (Now >= NextLine && GetLineCount() < MaxLines)
	{
		NextLine = Now + LineInterval;
		Cast<USizeBox>(AddLine())->SetWidthOverride(FullWidth);
	}
}
