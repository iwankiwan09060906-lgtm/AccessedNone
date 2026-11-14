#include "UI/MetaDraggableNumberWidget.h"
#include "UI/MetaErrorWindowWidget.h"
#include "UI/MetaUI.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"

void UMetaNumberDragOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);
	if (UMetaErrorWindowWidget* W = Window.Get())
	{
		W->DropOn({}); // outside / not a number → snap back
	}
}

TSharedRef<SWidget> UMetaDraggableNumberWidget::RebuildWidget()
{
	if (MetaUI::NeedsBuild(WidgetTree))
	{
		UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
		Root->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.04f));
		Root->SetPadding(FMargin(12.f, 4.f));
		Label = MetaUI::Text(WidgetTree, FText::GetEmpty(), 30, FLinearColor::White, TEXT("Mono"));
		Root->SetContent(Label);
		WidgetTree->RootWidget = Root;
		SetCursor(EMouseCursor::GrabHand);
	}
	return Super::RebuildWidget();
}

void UMetaDraggableNumberWidget::Init(UMetaErrorWindowWidget* InWindow, EMetaNumberSlot InSlot)
{
	Window = InWindow;
	NumberSlot = InSlot;
}

void UMetaDraggableNumberWidget::SetValue(int64 Value)
{
	if (Label)
	{
		FNumberFormattingOptions Opt;
		Opt.UseGrouping = false;
		Label->SetText(FText::Format(FText::FromString(TEXT("[ {0} ]")), FText::AsNumber(Value, &Opt)));
	}
}

FReply UMetaDraggableNumberWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Window.IsValid())
	{
		return FReply::Unhandled();
	}
	if (Window->IsHolding())
	{
		Window->DropOn(NumberSlot); // second click of click-click mode
		return FReply::Handled();
	}
	bPressed = true;
	bDragged = false;
	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply UMetaDraggableNumberWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bPressed && !bDragged && Window.IsValid() && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Window->BeginHold(NumberSlot); // first click of click-click mode
	}
	bPressed = false;
	return FReply::Handled().ReleaseMouseCapture();
}

void UMetaDraggableNumberWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	bDragged = true;
	bPressed = false;
	UMetaNumberDragOperation* Op = NewObject<UMetaNumberDragOperation>(this);
	Op->SourceSlot = NumberSlot;
	Op->Window = Window;
	Op->Pivot = EDragPivot::CenterCenter;
	Op->DefaultDragVisual = MetaUI::Text(WidgetTree, Label->GetText(), 30, MetaUI::Gold, TEXT("Mono"));
	OutOperation = Op;
	Window->BeginGrab(NumberSlot);
}

bool UMetaDraggableNumberWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (Cast<UMetaNumberDragOperation>(InOperation) && Window.IsValid())
	{
		Window->DropOn(NumberSlot);
		return true;
	}
	return false;
}

void UMetaDraggableNumberWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (Label) { Label->SetColorAndOpacity(FSlateColor(MetaUI::Gold)); }
}

void UMetaDraggableNumberWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (Label) { Label->SetColorAndOpacity(FSlateColor(FLinearColor::White)); }
}
