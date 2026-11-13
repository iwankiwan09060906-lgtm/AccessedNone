#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Characters/BossDataAsset.h"
#include "BossPatternComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossPatternPhase, FName, PatternId, bool, bWindupStarted);

/**
 * Boss AI without Behavior Trees (spec 0.3/8.6). Ticked, so the owner's CustomTimeDilation (E6 slomo)
 * slows windups, cooldowns and movement alike.
 */
UCLASS(ClassGroup=(Meta), meta=(BlueprintSpawnableComponent))
class ACCESSEDNONE_API UBossPatternComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossPatternComponent();

	UFUNCTION(BlueprintCallable, Category="Boss") void SetPatternSet(const TArray<FBossPattern>& InPatterns);
	UFUNCTION(BlueprintCallable, Category="Boss") void Start(AActor* InTarget);
	UFUNCTION(BlueprintCallable, Category="Boss") void Pause() { bPaused = true; }
	UFUNCTION(BlueprintCallable, Category="Boss") void Resume() { bPaused = false; }
	/** Stops and forgets the current attack and cooldowns. */
	UFUNCTION(BlueprintCallable, Category="Boss") void Stop();

	/** Scales Recovery (E4: the lone twin recovers twice as slowly). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") float RecoveryScale = 1.f;
	/** Distance at which the boss stops walking toward the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss") float ApproachAcceptance = 150.f;

	/** Windup start (true) and active start (false) — BP drives the telegraph glow. */
	UPROPERTY(BlueprintAssignable) FOnBossPatternPhase OnPatternPhase;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	enum class EPhase : uint8 { Idle, Windup, Active, Recovery };

	int32 PickPattern(float Distance) const;
	void BeginPhase(EPhase NewPhase, float Duration);
	void ApplyHits();
	void MoveTowardTarget();
	void StopMoving();

	TArray<FBossPattern> Patterns;
	TArray<double> NextReadyTime;
	TWeakObjectPtr<AActor> Target;
	EPhase Phase = EPhase::Idle;
	int32 Current = INDEX_NONE;
	float PhaseTimeLeft = 0.f;
	/** Accumulated owner-dilated time; cooldowns are measured in it. */
	double LocalTime = 0.0;
	bool bRunning = false;
	bool bPaused = false;
};
