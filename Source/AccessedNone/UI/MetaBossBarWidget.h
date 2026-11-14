#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaBossBarWidget.generated.h"

class AMetaBossBase;
class UProgressBar;
class UTextBlock;

/** WBP_BossBar: boss name + HP bar, bottom center (spec 4.1). */
UCLASS()
class ACCESSEDNONE_API UMetaBossBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetBoss(AMetaBossBase* InBoss);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TWeakObjectPtr<AMetaBossBase> Boss;
	UPROPERTY() TObjectPtr<UTextBlock> NameText;
	UPROPERTY() TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY() TObjectPtr<UProgressBar> Trail;
	double TrailHold = 0.0;
};
