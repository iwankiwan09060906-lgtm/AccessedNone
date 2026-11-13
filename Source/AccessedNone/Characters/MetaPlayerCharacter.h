#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
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

	UPROPERTY(EditDefaultsOnly, Category="Combat") TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

private:
	AActor* FindLockOnTarget() const;
	void ClearLockOn();
	void UpdateMoveSpeed();
	void TickStamina(float DeltaSeconds);
	void TickLockOn(float DeltaSeconds);

	TWeakObjectPtr<AActor> LockOnTarget;
	int32 HealFlaskCount = 0;
	double LastStaminaSpendTime = -1000.0;
	FVector AttackExtent = FVector(100.f, 100.f, 90.f);
	FVector AttackOffset = FVector(120.f, 0.f, 0.f);
	bool bDashing = false;
	bool bHealing = false;
};
