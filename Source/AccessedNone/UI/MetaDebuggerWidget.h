#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaDebuggerWidget.generated.h"

class UBorder;
class UVerticalBox;
class UTextBlock;
class USoundBase;

/**
 * WBP_Debugger: bottom-right terminal (spec 8.8). Messages from UHintSubsystem::OnDebuggerMessage are queued and
 * typed at CharSeconds per character in real time (unaffected by time dilation). Hidden until the first message.
 */
UCLASS()
class ACCESSEDNONE_API UMetaDebuggerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable) void Enqueue(const FText& Message);
	UFUNCTION(BlueprintPure) bool IsIdle() const { return Queue.IsEmpty() && !Current; }

	UPROPERTY(EditDefaultsOnly, Category="Debugger") float CharSeconds = 0.03f;
	/** Fades out after this long with nothing to type. */
	UPROPERTY(EditDefaultsOnly, Category="Debugger") float IdleHideSeconds = 12.f;
	UPROPERTY(EditDefaultsOnly, Category="Debugger") TSoftObjectPtr<USoundBase> KeyClickSound;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION() void HandleMessage(const FText& Message);

	UPROPERTY() TObjectPtr<UBorder> Frame;
	UPROPERTY() TObjectPtr<UVerticalBox> Lines;
	UPROPERTY() TObjectPtr<UTextBlock> Current;
	UPROPERTY() TObjectPtr<USoundBase> ClickSound;

	TArray<FString> Queue;
	FString CurrentFull;
	int32 Typed = 0;
	double LineStart = 0.0;
	double LastActivity = 0.0;

public:
	UMetaDebuggerWidget(const FObjectInitializer& Init);
};
