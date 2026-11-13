#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaTempMenuWidget.generated.h"

/** shortcut: placeholder ESC menu for S1-T07 verification; replaced by WBP_Settings (UMetaSettingsWidget) in S2-T03. */
UCLASS()
class ACCESSEDNONE_API UMetaTempMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
