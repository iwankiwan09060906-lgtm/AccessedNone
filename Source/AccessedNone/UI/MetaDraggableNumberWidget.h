#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/MetaErrorWindowState.h"
#include "MetaDraggableNumberWidget.generated.h"

class UMetaErrorWindowWidget;
class UTextBlock;

/** Carries the source slot while a number is dragged (spec 8.8). Dropping anywhere but a number widget cancels → snap back. */
UCLASS()
class ACCESSEDNONE_API UMetaNumberDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	EMetaNumberSlot SourceSlot = EMetaNumberSlot::Numerator;
	TWeakObjectPtr<UMetaErrorWindowWidget> Window;

	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
};

/** One grabbable number of the error window. Drag & drop, or click to pick up and click again to place. */
UCLASS()
class ACCESSEDNONE_API UMetaDraggableNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Init(UMetaErrorWindowWidget* InWindow, EMetaNumberSlot InSlot);
	void SetValue(int64 Value);
	EMetaNumberSlot GetNumberSlot() const { return NumberSlot; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	TWeakObjectPtr<UMetaErrorWindowWidget> Window;
	EMetaNumberSlot NumberSlot = EMetaNumberSlot::Numerator;
	UPROPERTY() TObjectPtr<UTextBlock> Label;
	bool bPressed = false;
	bool bDragged = false;
};
