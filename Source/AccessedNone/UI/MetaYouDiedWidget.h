#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaYouDiedWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

/**
 * WBP_YouDied: gameplay death screen (user revision, replaces the auto-respawn). The world stays almost fully visible,
 * a soft dark band fades in behind a crimson "YOU DIED", then "다시하기" / "메인 메뉴로" appear below. Real-time driven.
 * The opening's scripted death keeps UMetaDeathScreenWidget ("Player = None", spec 4.5).
 */
UCLASS()
class ACCESSEDNONE_API UMetaYouDiedWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UButton* MakeButton(const TCHAR* Key);

	UFUNCTION() void HandleRetry();
	UFUNCTION() void HandleMainMenu();

	UPROPERTY() TObjectPtr<UBorder> Shade;
	UPROPERTY() TObjectPtr<UWidget> BandStack;
	UPROPERTY() TObjectPtr<UTextBlock> Title;
	UPROPERTY() TObjectPtr<UWidget> Buttons;
	double StartTime = 0.0;
};
