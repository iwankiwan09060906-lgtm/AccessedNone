#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrologueDirector.generated.h"

class ADefenseEntity;
class UAudioComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInstanceDynamic;
class UMetaErrorWindowWidget;
class UMetaBossBarOverflowWidget;
class UGameplayAbility;
struct FAbilityEndedData;

UENUM(BlueprintType)
enum class EProloguePhase : uint8
{
	Fight,      // 4.1 fake boss + tutorial
	Clear,      // 4.1 DEMON VANQUISHED
	Crack,      // 4.2 tape stop, glitch, sky crack, entities fall
	Logs,       // 4.3 system logs / dialogue
	Execution,  // 4.4 forced defeat
	Dead,       // 4.5 Player = None + error window
	Done
};

/**
 * Drives the whole opening in L_Prologue_FlowerField (spec 4.1–4.5). Scene actors are found by tag:
 * "SkyCrack" (static mesh with M_SkyCrack), "HolyLight" (light). The fake boss is any boss in encounter FightEncounterId.
 */
UCLASS()
class ACCESSEDNONE_API APrologueDirector : public AActor
{
	GENERATED_BODY()

public:
	APrologueDirector();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure) EProloguePhase GetPhase() const { return Phase; }
	UFUNCTION(BlueprintPure) UMetaErrorWindowWidget* GetErrorWindow() const { return ErrorWindow; }

	UPROPERTY(EditAnywhere, Category="Prologue") FName FightEncounterId = TEXT("P1");
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<USoundBase> Music;
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<UParticleSystem> LightningFX;
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<UParticleSystem> SlashFX;
	UPROPERTY(EditAnywhere, Category="Prologue") int32 EntityCount = 6;
	UPROPERTY(EditAnywhere, Category="Prologue") float HolyLightIntensity = 60000.f;
	/** Seconds per tutorial step before it advances on its own. */
	UPROPERTY(EditAnywhere, Category="Prologue") float TutorialStepTimeout = 12.f;

protected:
	virtual void BeginPlay() override;

private:
	void EnterPhase(EProloguePhase NewPhase);
	bool Passed(float Seconds) const { return PhaseTime >= Seconds && PrevPhaseTime < Seconds; }
	void TickFight(float Dt);
	void TickClear();
	void TickCrack();
	void TickLogs();
	void TickExecution();
	void TickDead();
	void SetTutorialStep(int32 Step);
	void SpawnEntities();
	void KillPlayer();

	UFUNCTION() void HandleEncounterEnded(FName EncounterId);
	UFUNCTION() void HandleErrorWindowFinished();
	void HandleAbilityActivated(UGameplayAbility* Ability);

	class AMetaPlayerCharacter* GetPlayer() const;
	class UMetaHUDWidget* GetHUD() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> MusicComponent;
	UPROPERTY() TObjectPtr<AActor> CrackActor;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CrackMID;
	UPROPERTY() TObjectPtr<class ULightComponent> HolyLight;
	UPROPERTY() TArray<TObjectPtr<ADefenseEntity>> Entities;
	UPROPERTY() TObjectPtr<ADefenseEntity> Executor;
	UPROPERTY() TObjectPtr<UMetaErrorWindowWidget> ErrorWindow;
	UPROPERTY() TObjectPtr<UMetaBossBarOverflowWidget> OverflowBar;

	EProloguePhase Phase = EProloguePhase::Fight;
	float PhaseTime = 0.f;
	float PrevPhaseTime = -1.f;
	int32 TutorialStep = -1;
	float TutorialStepTime = 0.f;
	float MoveTime = 0.f;
	float NextLightning = 0.f;
	float DeadSince = -1.f;
	bool bPhaseEntered = false;
};
