#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "GimmickComponent.generated.h"

class UGimmickCondition;

UENUM(BlueprintType)
enum class EGimmickState : uint8 { Dormant, Armored, Breaking, Vulnerable, Defeated };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGimmickStateChanged, EGimmickState, OldState, EGimmickState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHintTierChanged, int32, Tier);

/** Boss gimmick state machine (spec 3.5 / 8.5). */
UCLASS(ClassGroup=(Meta), meta=(BlueprintSpawnableComponent))
class ACCESSEDNONE_API UGimmickComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** All must be satisfied (AND). Empty = never breaks (e.g. the fake boss). */
	UPROPERTY(EditAnywhere, Instanced, Category="Gimmick") TArray<TObjectPtr<UGimmickCondition>> Conditions;
	/** Forced to these (unfavourable) values when the encounter starts. */
	UPROPERTY(EditAnywhere, Category="Gimmick") TMap<EMetaSettingKey, float> SettingsToResetOnStart;
	UPROPERTY(EditAnywhere, Category="Gimmick", meta=(ClampMin="0")) float BreakingDuration = 2.0f;
	UPROPERTY(EditAnywhere, Category="Gimmick") bool bLatchOnceBroken = true;

	UFUNCTION(BlueprintCallable, Category="Gimmick") void BeginEncounter();
	/** Player died: back to Dormant. */
	UFUNCTION(BlueprintCallable, Category="Gimmick") void ResetEncounter();
	UFUNCTION(BlueprintCallable, Category="Gimmick") void NotifyConditionChanged();
	UFUNCTION(BlueprintCallable, Category="Gimmick") void NotifyDefeated();
	UFUNCTION(BlueprintPure, Category="Gimmick") EGimmickState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="Gimmick") bool AreConditionsSatisfied() const;

	void SetHintTier(int32 Tier);
	UFUNCTION(BlueprintPure, Category="Gimmick") int32 GetHintTier() const { return HintTier; }

	/** Name shown in SYS_SETTING_OVERRIDE, e.g. "BP_Assassin_C" (the owner's class name). */
	FString GetDebugName() const;

	UPROPERTY(BlueprintAssignable) FOnGimmickStateChanged OnStateChanged;
	UPROPERTY(BlueprintAssignable) FOnHintTierChanged OnHintTierChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void SetState(EGimmickState NewState);
	void ActivateConditions();
	void DeactivateConditions();
	void FinishBreaking();

	EGimmickState State = EGimmickState::Dormant;
	int32 HintTier = 0;
	FTimerHandle BreakingTimer;
};
