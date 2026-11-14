#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/MetaErrorWindowState.h"
#include "MetaErrorWindowWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UProgressBar;
class UTextBlock;
class UMetaDraggableNumberWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnErrorWindowFinished);

/** WBP_ErrorWindow: Phase 5 "Fatal Error — Respawn.exe" (spec 4.5). Rules live in FMetaErrorWindowState. */
UCLASS()
class ACCESSEDNONE_API UMetaErrorWindowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by the number widgets.
	void BeginGrab(EMetaNumberSlot From);
	/** Click-click mode: the number follows the cursor until the next click. */
	void BeginHold(EMetaNumberSlot From);
	bool IsHolding() const { return bHolding; }
	void DropOn(TOptional<EMetaNumberSlot> Target);

	const FMetaErrorWindowState& GetState() const { return State; }

	/** Overflow sequence finished → the director moves on to Stage 1. */
	UPROPERTY(BlueprintAssignable) FOnErrorWindowFinished OnFinished;

	UPROPERTY(EditDefaultsOnly, Category="ErrorWindow") float HintFirstSeconds = 30.f;
	UPROPERTY(EditDefaultsOnly, Category="ErrorWindow") float HintSecondSeconds = 60.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UFUNCTION() void HandleDeniedButton();
	void RefreshNumbers();
	void TickOverflow(double Now);

	UPROPERTY() TObjectPtr<UCanvasPanel> Root;
	UPROPERTY() TObjectPtr<UBorder> Window;
	UPROPERTY() TObjectPtr<UMetaDraggableNumberWidget> NumeratorWidget;
	UPROPERTY() TObjectPtr<UMetaDraggableNumberWidget> DenominatorWidget;
	UPROPERTY() TObjectPtr<UProgressBar> Progress;
	UPROPERTY() TObjectPtr<UTextBlock> PercentText;
	UPROPERTY() TObjectPtr<UTextBlock> DeniedText;
	UPROPERTY() TObjectPtr<UTextBlock> HeldVisual;

	FMetaErrorWindowState State;
	double OpenTime = 0.0;
	double LastTick = 0.0;
	double ShakeEnd = 0.0;
	double DeniedEnd = 0.0;
	double OverflowStart = 0.0;
	double NextPopup = 0.0;
	int32 PopupCount = 0;
	int32 HintsSent = 0;
	bool bHolding = false;
	bool bFinished = false;
};
