#include "UI/MetaHUDWidget.h"
#include "UI/MetaUI.h"
#include "UI/MetaBossBarWidget.h"
#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaInteractable.h"
#include "Core/MetaText.h"
#include "Meta/EncounterSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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

		// Bottom-left status (user revision): HP + flasks on the right, stamina below, on a fading dark backdrop.
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
		Backdrop->SetBrush(MetaUI::TextureBrush(TEXT("T_UI_GradientH"), FLinearColor(0.f, 0.f, 0.f, 0.8f)));
		Backdrop->SetPadding(FMargin(32.f, 22.f, 140.f, 22.f));
		UVerticalBox* Status = WidgetTree->ConstructWidget<UVerticalBox>();
		Backdrop->SetContent(Status);

		UHorizontalBox* HealthRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Status->AddChildToVerticalBox(HealthRow);
		HealthRow->AddChildToHorizontalBox(MetaUI::FramedBar(WidgetTree, HealthBarWidth, 24.f, FLinearColor(0.62f, 0.04f, 0.05f), HealthBar, HealthTrail))
			->SetVerticalAlignment(VAlign_Center);
		UHorizontalBox* Flasks = WidgetTree->ConstructWidget<UHorizontalBox>();
		HealthRow->AddChildToHorizontalBox(Flasks)->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
		for (int32 i = 0; i < UAccessedNoneSettings::Get().HealFlaskMax; ++i)
		{
			UImage* Icon = WidgetTree->ConstructWidget<UImage>();
			Icon->SetBrush(MetaUI::TextureBrush(TEXT("T_UI_Flask")));
			Icon->SetDesiredSizeOverride(FVector2D(30.f, 45.f));
			Flasks->AddChildToHorizontalBox(Icon)->SetPadding(FMargin(0.f, 0.f, 3.f, 0.f));
			FlaskIcons.Add(Icon);
		}
		FlaskText = MetaUI::Text(WidgetTree, FText::AsNumber(0), 22, MetaUI::Gold, TEXT("Bold"));
		UHorizontalBoxSlot* CountSlot = Flasks->AddChildToHorizontalBox(FlaskText);
		CountSlot->SetVerticalAlignment(VAlign_Bottom);
		CountSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));

		Status->AddChildToVerticalBox(MetaUI::FramedBar(WidgetTree, StaminaBarWidth, 11.f, FLinearColor(0.42f, 0.58f, 0.22f), StaminaBar, StaminaTrail))
			->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		Place(Canvas, Backdrop, FVector2D(0, 1), FVector2D(0, 1), FVector2D(0, -28));
		StatusRoot = Backdrop;

		TutorialBox = WidgetTree->ConstructWidget<UBorder>();
		TutorialBox->SetBrush(MetaUI::TextureBrush(TEXT("T_UI_GradientH"), FLinearColor(0.f, 0.f, 0.f, 0.75f)));
		TutorialBox->SetPadding(FMargin(28.f, 12.f, 80.f, 12.f));
		TutorialText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 18, FLinearColor(0.92f, 0.88f, 0.78f));
		TutorialBox->SetContent(TutorialText);
		TutorialBox->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, TutorialBox, FVector2D(0, 0.55f), FVector2D(0, 0.5f), FVector2D::ZeroVector);

		BossBar = CreateWidget<UMetaBossBarWidget>(this, MetaUI::WidgetClass<UMetaBossBarWidget>(TEXT("WBP_BossBar")));
		BossBar->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, BossBar, FVector2D(0.5f, 1), FVector2D(0.5f, 0), FVector2D(0, -190)); // top-aligned: a nested user widget has no desired size on the first layout pass

		LogBox = WidgetTree->ConstructWidget<UVerticalBox>();
		Place(Canvas, LogBox, FVector2D(0, 0), FVector2D(0, 0), FVector2D(40, 130));

		BannerText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 64, MetaUI::Gold);
		BannerText->SetJustification(ETextJustify::Center);
		Place(Canvas, BannerText, FVector2D(0.5f, 0.42f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		PromptText = MetaUI::Text(WidgetTree, FText::GetEmpty(), 20, FLinearColor(0.93f, 0.89f, 0.8f));
		PromptText->SetShadowOffset(FVector2D(1.f, 1.f));
		Place(Canvas, PromptText, FVector2D(0.5f, 0.72f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		Crosshair = MetaUI::Text(WidgetTree, FText::FromString(TEXT("+")), 30, FLinearColor(1.f, 1.f, 1.f, 0.85f), TEXT("Bold"));
		Crosshair->SetShadowOffset(FVector2D(1.f, 1.f));
		Crosshair->SetVisibility(ESlateVisibility::Collapsed);
		Place(Canvas, Crosshair, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

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

void UMetaHUDWidget::ShowToast(const FText& Text, float Seconds, FLinearColor Color)
{
	if (ToastText)
	{
		ToastText->SetText(Text);
		ToastText->SetColorAndOpacity(FSlateColor(Color));
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
	if (!bVisible)
	{
		BossBar->SetVisibility(V);
	}
}

void UMetaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = MetaUI::Now();

	if (AMetaPlayerCharacter* P = Cast<AMetaPlayerCharacter>(GetOwningPlayerPawn()))
	{
		const UMetaCombatAttributeSet* A = P->GetAttributes();
		const float HP = A->GetMaxHealth() > 0.f ? A->GetHealth() / A->GetMaxHealth() : 0.f;
		const float SP = A->GetMaxStamina() > 0.f ? A->GetStamina() / A->GetMaxStamina() : 0.f;
		HealthBar->SetPercent(HP);
		StaminaBar->SetPercent(SP);
		HealthTrail->SetPercent(MetaUI::StepTrail(HealthTrail->GetPercent(), HP, InDeltaTime, HealthTrailHold));
		StaminaTrail->SetPercent(MetaUI::StepTrail(StaminaTrail->GetPercent(), SP, InDeltaTime, StaminaTrailHold));
		// "[E] ..." for the nearest usable thing.
		const IMetaInteractable* Usable = P->IsDead() ? nullptr : P->FindInteractable();
		PromptText->SetText(Usable ? Usable->GetInteractPrompt() : FText::GetEmpty());

		const bool bAim = P->IsRanged() && !P->IsDead() && bStatusVisible;
		Crosshair->SetVisibility(bAim ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bAim)
		{
			AActor* Target = nullptr;
			bool bHit = false;
			P->GetAimPoint(Target, bHit);
			Crosshair->SetColorAndOpacity(FSlateColor(Target ? MetaUI::Error : FLinearColor(1.f, 1.f, 1.f, 0.85f)));
		}
		const int32 Flasks = P->GetHealFlaskCount();
		FlaskText->SetText(FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(Flasks)));
		for (int32 i = 0; i < FlaskIcons.Num(); ++i)
		{
			FlaskIcons[i]->SetRenderOpacity(i < Flasks ? 1.f : 0.18f); // used flasks stay as empty silhouettes
		}
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
