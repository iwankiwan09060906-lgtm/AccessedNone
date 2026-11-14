#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MetaGameMode.generated.h"

UCLASS()
class ACCESSEDNONE_API AMetaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMetaGameMode();
	/** Ensures the level has an AMetaPostProcessController (spec 8.9: one per level); despawns bosses of defeated encounters. */
	virtual void StartPlay() override;
	/** Continue: the first spawn goes to the saved breakpoint. */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Player died: YOU DIED screen; "다시하기" respawns at the active breakpoint, "메인 메뉴로" goes to the title. */
	void HandlePlayerDeath(class AMetaPlayerCharacter* Player);
	/** "다시하기" (tests call it directly). Respawns at once, no level reload (spec 6.2). */
	UFUNCTION(BlueprintCallable, Category="Respawn") void RequestRespawn();
	class UUserWidget* GetDeathScreen() const { return DeathScreen; }
	void SetActiveBreakpoint(class ABreakpointActor* Breakpoint) { ActiveBreakpoint = Breakpoint; }
	class ABreakpointActor* GetActiveBreakpoint() const { return ActiveBreakpoint; }

	/** The opening runs its own death flow (APrologueDirector). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn") bool bRespawnEnabled = true;

private:

	UPROPERTY() TObjectPtr<class ABreakpointActor> ActiveBreakpoint;
	UPROPERTY() TObjectPtr<class UUserWidget> DeathScreen;
	FTransform StartTransform;
	TWeakObjectPtr<class AMetaPlayerCharacter> DeadPlayer;
};
