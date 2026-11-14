#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
#include "DefenseEntity.generated.h"

/**
 * Opening-sequence defense entity (spec 4.2–4.4, user revision): a random Infinity Blade warrior in its unanimated
 * reference pose, invulnerable, no AI. Falls from the sky with plain character gravity; the phase 4 blast launches it.
 */
UCLASS()
class ACCESSEDNONE_API ADefenseEntity : public AMetaCharacterBase
{
	GENERATED_BODY()

public:
	ADefenseEntity();

	/** Throws the entity away from Origin (phase 4 explosion). */
	UFUNCTION(BlueprintCallable) void BlastAway(FVector Origin, float Strength);

	/** Meshes picked at random on spawn. */
	UPROPERTY(EditDefaultsOnly, Category="Defense") TArray<TSoftObjectPtr<USkeletalMesh>> Meshes;

protected:
	virtual void BeginPlay() override;
};
