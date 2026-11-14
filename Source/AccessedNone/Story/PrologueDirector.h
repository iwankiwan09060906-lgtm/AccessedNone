#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrologueDirector.generated.h"

class ADefenseEntity;
class AMetaBossBase;
class UAudioComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInstanceDynamic;
class UMetaErrorWindowWidget;
class UMetaDeathScreenWidget;
class UGameplayAbility;

UENUM(BlueprintType)
enum class EProloguePhase : uint8
{
	Fight,      // 4.1 tutorial, fake minions, walk to the dungeon, summoned fake boss
	Clear,      // 4.1 DEMON VANQUISHED
	Crack,      // 4.2 tape stop, glitch, the sky shatters, warriors rain down
	Logs,       // 4.3 system logs / dialogue
	Execution,  // 4.4 forced defeat: giant explosion
	Dead,       // 4.5 death screen + error window
	Done
};

/**
 * Drives the whole opening in L_Prologue_FlowerField (spec 4.1–4.5, revised by the user 2026-11-14). Scene actors are
 * found by tag: "SkyCrack" (static mesh with M_SkyCrack), "HolyLight" (light). The fake boss belongs to the encounter
 * trigger with FightEncounterId; it stays hidden until that encounter begins (summon). Minions use MinionEncounterId.
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
	UFUNCTION(BlueprintPure) int32 GetEntityCount() const { return Entities.Num(); }

	UPROPERTY(EditAnywhere, Category="Prologue") FName FightEncounterId = TEXT("P1");
	UPROPERTY(EditAnywhere, Category="Prologue") FName MinionEncounterId = TEXT("P1_Minions");
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<USoundBase> Music;
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<USoundBase> ExplosionSound;
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<UParticleSystem> SummonFX;
	/** Phase 4 blast: every system is spawned at the player, scaled by ExplosionScale. */
	UPROPERTY(EditAnywhere, Category="Prologue") TArray<TSoftObjectPtr<UParticleSystem>> ExplosionFX;
	UPROPERTY(EditAnywhere, Category="Prologue") TSoftObjectPtr<UParticleSystem> SideBlastFX;
	UPROPERTY(EditAnywhere, Category="Prologue") float ExplosionScale = 8.f;
	/** "Dozens" of falling warriors (user revision of 4.2). */
	UPROPERTY(EditAnywhere, Category="Prologue") int32 EntityCount = 40;
	UPROPERTY(EditAnywhere, Category="Prologue") float EntitySpawnInterval = 0.15f;
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
	void TickEntityRain();
	void SetTutorialStep(int32 Step);
	void PlaceCrackInView();
	void Explode();
	void KillPlayer();

	UFUNCTION() void HandleEncounterBegan(FName EncounterId);
	UFUNCTION() void HandleEncounterEnded(FName EncounterId);
	UFUNCTION() void HandleErrorWindowFinished();
	void HandleAbilityActivated(UGameplayAbility* Ability);

	class AMetaPlayerCharacter* GetPlayer() const;
	class UMetaHUDWidget* GetHUD() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UAudioComponent> MusicComponent;
	UPROPERTY() TObjectPtr<AActor> CrackActor;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CrackMID;
	UPROPERTY() TObjectPtr<class ULightComponent> HolyLight;
	UPROPERTY() TArray<TObjectPtr<AMetaBossBase>> SummonedBosses;
	UPROPERTY() TArray<TObjectPtr<ADefenseEntity>> Entities;
	UPROPERTY() TObjectPtr<UMetaErrorWindowWidget> ErrorWindow;
	UPROPERTY() TObjectPtr<UMetaDeathScreenWidget> DeathScreen;

	EProloguePhase Phase = EProloguePhase::Fight;
	float PhaseTime = 0.f;
	float PrevPhaseTime = -1.f;
	int32 TutorialStep = -1;
	float TutorialStepTime = 0.f;
	float MoveTime = 0.f;
	double NextEntityTime = -1.0;
	double FlashStart = -1.0;
	float DeadSince = -1.f;
	bool bPhaseEntered = false;
};
