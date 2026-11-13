#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MeleeHitboxComponent.generated.h"

/** Box overlap in the owner's local space. Used by player attacks and boss patterns (animation-free combat, 7.5). */
UCLASS(ClassGroup=(Meta), meta=(BlueprintSpawnableComponent))
class ACCESSEDNONE_API UMeleeHitboxComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Pawns overlapping a box of half-size Extent centred at LocalOffset (owner space). Excludes the owner. */
	UFUNCTION(BlueprintCallable, Category="Combat")
	TArray<AActor*> QueryHits(const FVector& Extent, const FVector& LocalOffset) const;
};
