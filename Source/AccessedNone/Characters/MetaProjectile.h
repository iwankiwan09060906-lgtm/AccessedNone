#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MetaProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UAbilitySystemComponent;

/**
 * Straight-flying projectile (Sparrow's arrows now; E5 daggers / enemy bullets later, spec 7.3).
 * Damages the first combatant it touches through GE_Damage (AttackPower × MotionValue), stops on level geometry.
 */
UCLASS()
class ACCESSEDNONE_API AMetaProjectile : public AActor
{
	GENERATED_BODY()

public:
	AMetaProjectile();

	void Launch(UAbilitySystemComponent* InSourceASC, const FVector& Direction, float Speed, float MotionValue, FGameplayTag DamageType,
		UParticleSystem* TrailFX, UParticleSystem* InHitFX);

	UPROPERTY(EditDefaultsOnly, Category="Projectile") float LifeSeconds = 1.5f;

private:
	UFUNCTION() void HandleOverlap(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION() void HandleHit(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit);
	void Impact(const FVector& Location);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UParticleSystemComponent> Trail;
	UPROPERTY() TObjectPtr<UParticleSystem> HitFX;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	float MotionValue = 1.f;
	FGameplayTag DamageType;
};
