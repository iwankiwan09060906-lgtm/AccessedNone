#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
#include "Gimmick/GimmickComponent.h"
#include "MetaBossBase.generated.h"

class UBossPatternComponent;
class UBossDataAsset;
class UMeleeHitboxComponent;

/** Defense entity ("boss" in dev docs). State visuals live in BP; rules live here. */
UCLASS(Abstract)
class ACCESSEDNONE_API AMetaBossBase : public AMetaCharacterBase
{
	GENERATED_BODY()

public:
	AMetaBossBase();

	/** Called by UEncounterSubsystem. HP is computed from this target's AttackPower right now (spec 3.1). */
	void StartEncounter(AActor* PlayerTarget);
	/** Player died: Dormant, full reset to spawn. */
	void ResetEncounter();

	UFUNCTION(BlueprintPure, Category="Boss") UGimmickComponent* GetGimmick() const { return Gimmick; }
	UFUNCTION(BlueprintPure, Category="Boss") UBossDataAsset* GetBossData() const { return BossData; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") TObjectPtr<UGimmickComponent> Gimmick;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") TObjectPtr<UBossPatternComponent> Patterns;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss") TObjectPtr<UMeleeHitboxComponent> Hitbox;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Boss") TObjectPtr<UBossDataAsset> BossData;

	/** Set by UEncounterSubsystem when this boss joins an encounter (training dummies restart it on revive). */
	FName EncounterId;

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath(const FGameplayEventData* Payload) override;

	UFUNCTION() virtual void HandleGimmickStateChanged(EGimmickState Old, EGimmickState New);

	UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnBreakingStarted();
	UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnVulnerable();
	UFUNCTION(BlueprintImplementableEvent, Category="Boss") void OnDefeated();

private:
	void RemoveStateEffects();
	void PlayDeathAndVanish();
	void Vanish();
	void Revive();
	void SetPresent(bool bPresent);
	bool IsDummy() const;

	FTimerHandle DeathTimer;

	TWeakObjectPtr<AActor> EncounterTarget;
	FTransform SpawnTransform;
};
