#include "UI/MetaDebuggerWidget.h"
#include "UI/MetaUI.h"
#include "Meta/HintSubsystem.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr int32 MaxLines = 6;
	constexpr int32 CharsPerClick = 2;
}

UMetaDebuggerWidget::UMetaDebuggerWidget(const FObjectInitializer& Init) : Super(Init)
{
	KeyClickSound = FSoftObjectPath(TEXT("/Game/Audio/SW_KeyClick.SW_KeyClick"));
}

TSharedRef<SWidget> UMetaDebuggerWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		// Full-screen transparent root, terminal in the bottom-right corner (spec 8.8).
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
		Root->SetBrushColor(FLinearColor::Transparent);
		Root->SetPadding(FMargin(32.f));
		Root->SetHorizontalAlignment(HAlign_Right);
		Root->SetVerticalAlignment(VAlign_Bottom);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(620.f);
		Root->SetContent(Size);
		Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(FLinearColor(0.f, 0.03f, 0.02f, 0.88f));
		Frame->SetPadding(FMargin(14.f, 10.f));
		Size->AddChild(Frame);
		Lines = WidgetTree->ConstructWidget<UVerticalBox>();
		Frame->SetContent(Lines);
		WidgetTree->RootWidget = Root;
		SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UMetaDebuggerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ClickSound = KeyClickSound.LoadSynchronous();
	if (UHintSubsystem* Hints = GetGameInstance()->GetSubsystem<UHintSubsystem>())
	{
		Hints->OnDebuggerMessage.AddUniqueDynamic(this, &UMetaDebuggerWidget::HandleMessage);
	}
}

void UMetaDebuggerWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		GI->GetSubsystem<UHintSubsystem>()->OnDebuggerMessage.RemoveDynamic(this, &UMetaDebuggerWidget::HandleMessage);
	}
	Super::NativeDestruct();
}

void UMetaDebuggerWidget::HandleMessage(const FText& Message)
{
	Enqueue(Message);
}

void UMetaDebuggerWidget::Enqueue(const FText& Message)
{
	Queue.Add(Message.ToString());
	LastActivity = MetaUI::Now();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMetaDebuggerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const double Now = MetaUI::Now();

	if (!Current && !Queue.IsEmpty())
	{
		if (Lines->GetChildrenCount() >= MaxLines)
		{
			Lines->RemoveChildAt(0);
		}
		CurrentFull = Queue[0];
		Queue.RemoveAt(0);
		Typed = 0;
		LineStart = Now;
		Current = MetaUI::Text(WidgetTree, FText::GetEmpty(), 16, MetaUI::Debugger, TEXT("Mono"));
		Current->SetAutoWrapText(true);
		Lines->AddChildToVerticalBox(Current);
	}

	if (Current)
	{
		const int32 Target = FMath::Min(CurrentFull.Len(), FMath::FloorToInt32((Now - LineStart) / FMath::Max(CharSeconds, 0.001f)));
		if (Target != Typed)
		{
			if (ClickSound && Target / CharsPerClick != Typed / CharsPerClick)
			{
				UGameplayStatics::PlaySound2D(this, ClickSound);
			}
			Typed = Target;
			Current->SetText(FText::FromString(CurrentFull.Left(Typed) + (Typed < CurrentFull.Len() ? TEXT("_") : TEXT(""))));
		}
		if (Typed >= CurrentFull.Len())
		{
			Current = nullptr;
		}
		LastActivity = Now;
	}
	else if (IsVisible() && Now - LastActivity > IdleHideSeconds)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
