#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaDeathScreenWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * WBP_DeathScreen (spec 4.5 ①, game-style): the screen darkens, a crimson band slides open, "Player = None" settles in
 * from a slight zoom with a chromatic-split glitch, and an engine error line types underneath. Real-time driven.
 */
UCLASS()
class ACCESSEDNONE_API UMetaDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UBorder> Shade;
	UPROPERTY() TObjectPtr<UBorder> Band;
	UPROPERTY() TObjectPtr<UWidget> TitleStack;
	UPROPERTY() TObjectPtr<UTextBlock> SplitA;
	UPROPERTY() TObjectPtr<UTextBlock> SplitB;
	UPROPERTY() TObjectPtr<UTextBlock> SubText;
	FString SubFull;
	double StartTime = 0.0;
};
