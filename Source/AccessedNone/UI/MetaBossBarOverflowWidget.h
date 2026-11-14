#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaBossBarOverflowWidget.generated.h"

class UVerticalBox;
class UWidget;

/**
 * WBP_BossBar_Overflow (spec 4.4 ②): a boss bar that grows past the right edge, then wraps to a new full line
 * every LineInterval seconds until MaxLines rows cover the top of the screen.
 */
UCLASS()
class ACCESSEDNONE_API UMetaBossBarOverflowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="Overflow") float FirstLineSeconds = 0.6f;
	UPROPERTY(EditDefaultsOnly, Category="Overflow") float LineInterval = 0.1f;
	UPROPERTY(EditDefaultsOnly, Category="Overflow") int32 MaxLines = 12;

	int32 GetLineCount() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UWidget* AddLine();

	UPROPERTY() TObjectPtr<UVerticalBox> Lines;
	double StartTime = 0.0;
	double NextLine = 0.0;
};
