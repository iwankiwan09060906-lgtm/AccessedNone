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
		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>();
		Root->SetWidthOverride(900.f);
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>();
		Root->AddChild(Box);

		NameText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 18, FLinearColor(0.9f, 0.88f, 0.82f));
		Box->AddChildToVerticalBox(NameText);

		USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>();
		BarSize->SetHeightOverride(14.f);
		HealthBar = WidgetTree->ConstructWidget<UProgressBar>();
		HealthBar->SetFillColorAndOpacity(FLinearColor(0.55f, 0.04f, 0.04f));
		BarSize->AddChild(HealthBar);
		Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(BarSize))->SetPadding(FMargin(0.f, 4.f));
		HealthBar->SetPercent(1.f);
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
		HealthBar->SetPercent(B->GetHealth() / B->GetMaxHealth());
	}
}
