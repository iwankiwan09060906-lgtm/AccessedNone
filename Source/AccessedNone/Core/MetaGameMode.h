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

	/** Player died: death screen, then respawn at the active breakpoint within RespawnMaxSeconds (spec 6.2). */
	void HandlePlayerDeath(class AMetaPlayerCharacter* Player);
	void SetActiveBreakpoint(class ABreakpointActor* Breakpoint) { ActiveBreakpoint = Breakpoint; }
	class ABreakpointActor* GetActiveBreakpoint() const { return ActiveBreakpoint; }

	/** The opening runs its own death flow (APrologueDirector). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Respawn") bool bRespawnEnabled = true;

private:
	void Respawn(TWeakObjectPtr<class AMetaPlayerCharacter> Player);

	UPROPERTY() TObjectPtr<class ABreakpointActor> ActiveBreakpoint;
	UPROPERTY() TObjectPtr<class UUserWidget> DeathScreen;
	FTransform StartTransform;
	FTimerHandle RespawnTimer;
};
