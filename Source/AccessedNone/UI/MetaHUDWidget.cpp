#include "UI/MetaHUDWidget.h"
#include "UI/MetaUI.h"
#include "UI/MetaBossBarWidget.h"
#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/MetaText.h"
#include "Meta/EncounterSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr int32 MaxLogLines = 8;
	constexpr double FloatingSeconds = 1.0;
	constexpr float FloatingRise = 80.f;

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* W, FVector2D Anchor, FVector2D Alignment, FVector2D Offset)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		S->SetAlignment(Alignment);
		S->SetPosition(Offset);
		S->SetAutoSize(true);
		return S;
	}
}

UProgressBar* UMetaHUDWidget::MakeBar(UVerticalBox* Box, const FLinearColor& Color, float Width)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(Width);
	Size->SetHeightOverride(12.f);
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>();
	Bar->SetFillColorAndOpacity(Color);
	Bar->SetPercent(1.f);
	Size->AddChild(Bar);
	Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Size))->SetPadding(FMargin(0.f, 2.f));
	return Bar;
}

TSharedRef<SWidget> UMetaHUDWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		Tint = WidgetTree->ConstructWidget<UBorder>();
		Tint->SetBrushColor(FLinearColor::Transparent);
		UCanvasPanelSlot* TintSlot = Canvas->AddChildToCanvas(Tint);
		TintSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		TintSlot->SetOffsets(FMargin(0.f));

		// Top-left: HP / FP / stamina.
		UVerticalBox* Status = WidgetTree->ConstructWidget<UVerticalBox>();
		HealthBar = MakeBar(Status, FLinearColor(0.6f, 0.05f, 0.05f), 360.f);
		FocusBar = MakeBar(Status, FLinearColor(0.1f, 0.2f, 0.7f), 240.f);
		StaminaBar = MakeBar(Status, FLinearColor(0.15f, 0.5f, 0.15f), 300.f);
		Place(Canvas, Status, FVector2D(0, 0), FVector2D(0, 0), FVector2D(40, 36));

		// Bottom-left: quick slot (heal flask).
		UBorder* QuickSlot = WidgetTree->ConstructWidget<UBorder>();
		QuickSlot->SetBrushColor(MetaUI::Panel);
		QuickSlot->SetPadding(FMargin(18.f, 10.f));
		FlaskText = MetaUI::Text(WidgetTree, FText::AsNumber(0), 22, MetaUI::Gold);
		QuickSlot->SetContent(FlaskText);
		Place(Canvas, QuickSlot, FVector2D(0, 1), FVector2D(0, 1), FVector2D(40, -40));

		TutorialBox = WidgetTree->ConstructWidget<UBorder>();
		TutorialBox->SetBrushColor(MetaUI::Panel);
		TutorialBox->SetPadding(FMargin(16.f, 10.f));
		TutorialText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 18);
		TutorialBox->SetContent(TutorialText);
		TutorialBox->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, TutorialBox, FVector2D(0, 1), FVector2D(0, 1), FVector2D(40, -110));

		BossBar = CreateWidget<UMetaBossBarWidget>(this, MetaUI::WidgetClass<UMetaBossBarWidget>(TEXT("WBP_BossBar")));
		BossBar->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, BossBar, FVector2D(0.5f, 1), FVector2D(0.5f, 0), FVector2D(0, -110)); // top-aligned: a nested user widget has no desired size on the first layout pass

		StatusRoot = Status;

		LogBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Place(Canvas, LogBox, FVector2D(0, 0), FVector2D(0, 0), FVector2D(40, 130));

		BannerText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 64, MetaUI::Gold);
		BannerText->SetJustification(ETextJustify::Center);
		Place(Canvas, BannerText, FVector2D(0.5f, 0.42f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		ToastText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 26, MetaUI::Error);
		Place(Canvas, ToastText, FVector2D(0.5f, 0.18f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UMetaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	DamageHandle = FMetaCombatEvents::OnDamage.AddUObject(this, &UMetaHUDWidget::HandleDamage);
}

void UMetaHUDWidget::NativeDestruct()
{
	FMetaCombatEvents::OnDamage.Remove(DamageHandle);
	Super::NativeDestruct();
}

void UMetaHUDWidget::HandleDamage(AActor* Target, float Damage, bool bImmune)
{
	if (!bDamageNumbers || !Target || Target->GetWorld() != GetWorld())
	{
		return;
	}
	const bool bPlayer = Target == GetOwningPlayerPawn();
	const FText Text = bImmune ? MetaText::Get(TEXT("UI_IMMUNE")) : FText::AsNumber(FMath::RoundToInt(Damage));
	ShowFloatingText(Target->GetActorLocation() + FVector(0, 0, 100), Text, bPlayer ? MetaUI::Error : FLinearColor::White);
}

void UMetaHUDWidget::ShowFloatingText(FVector WorldLocation, const FText& Text, FLinearColor Color, float Size)
{
	if (!Canvas)
	{
		return;
	}
	UTextBlock* T = MetaUI::Text(WidgetTree, Text, FMath::RoundToInt(Size), Color);
	T->SetShadowOffset(FVector2D(2, 2));
	T->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8f));
	Place(Canvas, T, FVector2D::ZeroVector, FVector2D(0.5f, 0.5f), FVector2D(-1000, -1000));
	Floating.Add({ T, WorldLocation, MetaUI::Now() });
}

void UMetaHUDWidget::ShowToast(const FText& Text, float Seconds)
{
	if (ToastText)
	{
		ToastText->SetText(Text);
		ToastEndTime = MetaUI::Now() + Seconds;
	}
}

void UMetaHUDWidget::ShowBanner(const FText& Text, FLinearColor Color, float Size)
{
	if (BannerText)
	{
		GlitchEnd = 0.0;
		BannerText->SetRenderTranslation(FVector2D::ZeroVector);
		BannerText->SetText(Text);
		BannerText->SetColorAndOpacity(FSlateColor(Color));
		BannerText->SetFont(MetaUI::Font(FMath::RoundToInt(Size), TEXT("Bold")));
	}
}

void UMetaHUDWidget::GlitchBanner(const FText& GlitchedText, float Seconds)
{
	GlitchTarget = GlitchedText;
	GlitchStart = MetaUI::Now();
	GlitchEnd = GlitchStart + Seconds;
}

void UMetaHUDWidget::PushLogLine(const FText& Line, FLinearColor Color)
{
	if (!LogBox)
	{
		return;
	}
	if (LogBox->GetChildrenCount() >= MaxLogLines)
	{
		LogBox->RemoveChildAt(0);
	}
	UTextBlock* T = MetaUI::Text(WidgetTree, Line, 15, Color, TEXT("Mono"));
	T->SetShadowOffset(FVector2D(1, 1));
	LogBox->AddChildToVerticalBox(T);
}

void UMetaHUDWidget::ClearLog()
{
	if (LogBox)
	{
		LogBox->ClearChildren();
	}
}

void UMetaHUDWidget::ShowTutorial(const FText& Text)
{
	if (TutorialBox)
	{
		TutorialText->SetText(Text);
		TutorialBox->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UMetaHUDWidget::SetScreenTint(FLinearColor Color)
{
	if (Tint)
	{
		Tint->SetBrushColor(Color);
	}
}

void UMetaHUDWidget::SetStatusVisible(bool bVisible)
{
	bStatusVisible = bVisible;
	const ESlateVisibility V = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	StatusRoot->SetVisibility(V);
	FlaskText->GetParent()->SetVisibility(V);
	if (!bVisible)
	{
		BossBar->SetVisibility(V);
	}
}

void UMetaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = MetaUI::Now();

	if (const AMetaPlayerCharacter* P = Cast<AMetaPlayerCharacter>(GetOwningPlayerPawn()))
	{
		const UMetaCombatAttributeSet* A = P->GetAttributes();
		HealthBar->SetPercent(A->GetMaxHealth() > 0.f ? A->GetHealth() / A->GetMaxHealth() : 0.f);
		StaminaBar->SetPercent(A->GetMaxStamina() > 0.f ? A->GetStamina() / A->GetMaxStamina() : 0.f);
		FlaskText->SetText(FText::AsNumber(P->GetHealFlaskCount()));
	}

	const UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>();
	AMetaBossBase* Boss = Encounters ? Encounters->GetPrimaryBoss() : nullptr;
	if (Boss && (!Boss->GetBossData() || Boss->GetBossData()->DisplayNameKey.IsNone()))
	{
		Boss = nullptr; // unnamed minions get no boss bar (souls-like)
	}
	BossBar->SetBoss(Boss);
	BossBar->SetVisibility(Boss && bStatusVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (ToastEndTime > 0.0 && Now > ToastEndTime)
	{
		ToastText->SetText(FText::GetEmpty());
		ToastEndTime = 0.0;
	}

	if (GlitchEnd > 0.0)
	{
		if (Now >= GlitchEnd)
		{
			BannerText->SetText(GlitchTarget);
			BannerText->SetRenderTranslation(FVector2D::ZeroVector);
			GlitchEnd = 0.0;
		}
		else
		{
			// Jitter only the banner (spec 12.3: glitches stay local, never full-screen flashes).
			BannerText->SetRenderTranslation(FVector2D(FMath::FRandRange(-10.f, 10.f), FMath::FRandRange(-4.f, 4.f)));
			if (FMath::FRand() < 0.3f)
			{
				BannerText->SetText(GlitchTarget);
			}
		}
	}

	APlayerController* PC = GetOwningPlayer();
	for (int32 i = Floating.Num() - 1; i >= 0; --i)
	{
		FFloating& F = Floating[i];
		const double Age = Now - F.StartTime;
		UTextBlock* T = F.Widget.Get();
		if (!T || Age > FloatingSeconds)
		{
			if (T) { T->RemoveFromParent(); }
			Floating.RemoveAtSwap(i);
			continue;
		}
		const float Alpha = static_cast<float>(Age / FloatingSeconds);
		FVector2D ScreenPos;
		if (PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, F.WorldLocation + FVector(0, 0, FloatingRise * Alpha), ScreenPos, false))
		{
			Cast<UCanvasPanelSlot>(T->Slot)->SetPosition(ScreenPos);
			T->SetRenderOpacity(1.f - Alpha * Alpha);
		}
	}
}
