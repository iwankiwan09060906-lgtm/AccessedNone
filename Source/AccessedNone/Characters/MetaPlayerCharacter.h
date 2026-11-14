#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
#include "GameplayTagContainer.h"
#include "MetaPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMeleeHitboxComponent;
class UGameplayAbility;

/** The knight. Input arrives from AMetaPlayerController (so AStatuePawn inherits everything). */
UCLASS()
class ACCESSEDNONE_API AMetaPlayerCharacter : public AMetaCharacterBase
{
	GENERATED_BODY()

public:
	AMetaPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// --- Input entry points (called by the controller) ---
	void Move(const FVector2D& Input);
	void Look(const FVector2D& Input);
	void TryJump();
	void SetDashing(bool bWantsDash);
	void ToggleLockOn();
	/** Swaps the visual hero (UAccessedNoneSettings::Heroes[Index]) and its attack reach. */
	UFUNCTION(BlueprintCallable, Category="Hero") void SetHero(int32 Index);
	FVector GetAttackExtent() const { return AttackExtent; }
	FVector GetAttackOffset() const { return AttackOffset; }
	bool ActivateAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	/** Current hero's light combo step or heavy attack animation, stretched to the attack's duration. */
	void PlayAttackMotion(bool bHeavy, float TotalSeconds);
	// --- Ranged hero (Sparrow) ---
	UFUNCTION(BlueprintPure, Category="Hero") bool IsRanged() const;
	/** Where an arrow would land: the lock-on target, else the camera-center trace (≤ AimRange). OutTarget = combatant under the aim. */
	FVector GetAimPoint(AActor*& OutTarget, bool& bOutHit) const;
	void FireProjectile(float MotionValue, FGameplayTag DamageType, bool bHeavy);
	/** While a ranged attack runs the body turns to the aim point, then back to facing the movement direction. */
	void SetAttackFacing(bool bFacing);

	/** Somersault the mesh along WorldDir over Seconds (the roll has no animation asset). */
	void StartRollVisual(const FVector& WorldDir, float Seconds);

	// --- Used by abilities ---
	/** Spends stamina if any is left (souls-style: an action is allowed while Stamina > 0). */
	bool TrySpendStamina(float Cost);
	bool ConsumeHealFlask();
	void SetHealing(bool bHealing);
	UMeleeHitboxComponent* GetHitbox() const { return Hitbox; }

	UFUNCTION(BlueprintPure, Category="Combat") bool IsLockedOn() const { return LockOnTarget.IsValid(); }
	UFUNCTION(BlueprintPure, Category="Combat") AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }
	UFUNCTION(BlueprintPure, Category="Combat") int32 GetHealFlaskCount() const { return HealFlaskCount; }
	UFUNCTION(BlueprintCallable, Category="Combat") void RefillHealFlasks();

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath(const FGameplayEventData* Payload) override;

	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, Category="Camera") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<UMeleeHitboxComponent> Hitbox;
	/** Ground/enemy marker at the aim point while a ranged hero is active. */
	UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<class UParticleSystemComponent> AimMarker;

	UPROPERTY(EditDefaultsOnly, Category="Combat") TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

private:
	AActor* FindLockOnTarget() const;
	void ClearLockOn();
	void UpdateMoveSpeed();
	void TickStamina(float DeltaSeconds);
	void TickLockOn(float DeltaSeconds);
	void TickRoll(float DeltaSeconds);
	void TickAim();
	const struct FHeroDef* GetHeroDef() const;

	TWeakObjectPtr<AActor> LockOnTarget;
	int32 HealFlaskCount = 0;
	double LastStaminaSpendTime = -1000.0;
	FVector AttackExtent = FVector(100.f, 100.f, 90.f);
	FVector AttackOffset = FVector(120.f, 0.f, 0.f);
	FVector RollAxisLocal = FVector::RightVector;
	FTransform MeshBaseTransform;
	float RollElapsed = -1.f;
	float RollSeconds = 0.f;
	int32 HeroIndex = 0;
	int32 ComboStep = 0;
	double LastLightAttackTime = -1000.0;
	bool bDashing = false;
	bool bAttackFacing = false;
	bool bHealing = false;
};
