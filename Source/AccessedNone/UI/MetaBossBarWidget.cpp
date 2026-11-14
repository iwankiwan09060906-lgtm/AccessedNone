#include "UI/MetaBossBarWidget.h"
#include "UI/MetaUI.h"
#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Core/MetaText.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

TSharedRef<SWidget> UMetaBossBarWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
		NameText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 18, FLinearColor(0.93f, 0.89f, 0.8f));
		NameText->SetFont(MetaUI::Font(18, TEXT("Regular"), 120));
		NameText->SetShadowOffset(FVector2D(1.f, 1.f));
		Root->AddChildToVerticalBox(NameText)->SetPadding(FMargin(2.f, 0.f, 0.f, 4.f));
		Root->AddChildToVerticalBox(MetaUI::FramedBar(WidgetTree, 820.f, 12.f, FLinearColor(0.58f, 0.03f, 0.04f), HealthBar, Trail));
		WidgetTree->RootWidget = Root;
	}
	return Super::RebuildWidget();
}

void UMetaBossBarWidget::SetBoss(AMetaBossBase* InBoss)
{
	if (Boss.Get() == InBoss)
	{
		return;
	}
	Boss = InBoss;
	const UBossDataAsset* Data = InBoss ? InBoss->GetBossData() : nullptr;
	if (NameText)
	{
		NameText->SetText(Data && !Data->DisplayNameKey.IsNone() ? MetaText::Get(*Data->DisplayNameKey.ToString()) : FText::GetEmpty());
	}
}

void UMetaBossBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (const AMetaBossBase* B = Boss.Get(); B && HealthBar && B->GetMaxHealth() > 0.f)
	{
		const float Pct = B->GetHealth() / B->GetMaxHealth();
		HealthBar->SetPercent(Pct);
		Trail->SetPercent(MetaUI::StepTrail(Trail->GetPercent(), Pct, InDeltaTime, TrailHold));
	}
}
