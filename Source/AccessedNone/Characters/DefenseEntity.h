#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
#include "DefenseEntity.generated.h"

class UPoseableMeshComponent;

/**
 * Opening-sequence defense entity (spec 4.2–4.4, user revision): a random Infinity Blade warrior frozen in a random
 * contorted pose (the pack has no animations, so limb bones are posed procedurally), invulnerable, no AI.
 * Tumbles while falling with plain character gravity; the phase 4 blast launches it again.
 */
UCLASS()
class ACCESSEDNONE_API ADefenseEntity : public AMetaCharacterBase
{
	GENERATED_BODY()

public:
	ADefenseEntity();
	virtual void Tick(float DeltaSeconds) override;

	/** Throws the entity away from Origin (phase 4 explosion). */
	UFUNCTION(BlueprintCallable) void BlastAway(FVector Origin, float Strength);

	/** Meshes picked at random on spawn. */
	UPROPERTY(EditDefaultsOnly, Category="Defense") TArray<TSoftObjectPtr<USkeletalMesh>> Meshes;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyRandomPose();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UPoseableMeshComponent> Body;
	FQuat Tumble = FQuat::Identity;
	FVector SpinAxis = FVector::UpVector;
	float SpinSpeed = 0.f; // rad/s
	FVector BodyOffset = FVector(0.f, 0.f, -96.f);
	FQuat BodyBaseRotation = FQuat::Identity;
};
