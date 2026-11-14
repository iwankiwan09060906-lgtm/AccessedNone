#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "MetaTitleWidget.generated.h"

class UButton;

/** WBP_Title (spec 6.4): New Game / Continue (only with a save) / Quit. */
UCLASS()
class ACCESSEDNONE_API UMetaTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fresh session state (virtual settings + hint counters), then the prologue. */
	UFUNCTION(BlueprintCallable) void NewGame();
	/** Restores the save into the subsystems and opens its level. */
	UFUNCTION(BlueprintCallable) void Continue();
	UFUNCTION(BlueprintCallable) void Quit();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UButton* AddButton(class UVerticalBox* Box, const TCHAR* Key);
};

/** L_Title game mode: no pawn, title widget. */
UCLASS()
class ACCESSEDNONE_API AMetaTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMetaTitleGameMode();
};

UCLASS()
class ACCESSEDNONE_API AMetaTitlePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
