#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaCharacterBase.h"
#include "DefenseEntity.generated.h"

class UMaterialInstanceDynamic;

/**
 * Opening-sequence defense entity (spec 4.2–4.4): unanimated T-pose mannequin with the grid material,
 * invulnerable, no AI. Falls from the sky with plain character gravity, slides without root motion,
 * and swings with deliberately missing frames.
 */
UCLASS()
class ACCESSEDNONE_API ADefenseEntity : public AMetaCharacterBase
{
	GENERATED_BODY()

public:
	ADefenseEntity();
	virtual void Tick(float DeltaSeconds) override;

	/** Position-only interpolation (no root motion). */
	UFUNCTION(BlueprintCallable) void SlideTo(FVector Target, float Seconds);
	/** Three snapped poses over SwingSeconds (spec 4.4 ③: 2–3 frame motion via transform snaps). */
	UFUNCTION(BlueprintCallable) void ChoppySwing();
	/** M_WorldGrid "ExplodeAmount": vertices shooting out along their normals. */
	UFUNCTION(BlueprintCallable) void SetExplode(float Amount);

	UPROPERTY(EditDefaultsOnly, Category="Defense") TSoftObjectPtr<UMaterialInterface> GridMaterial;
	UPROPERTY(EditDefaultsOnly, Category="Defense") float SwingSeconds = 0.25f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GridMID;
	FVector SlideFrom = FVector::ZeroVector;
	FVector SlideTarget = FVector::ZeroVector;
	float SlideDuration = 0.f;
	float SlideElapsed = -1.f;
	float SwingElapsed = -1.f;
	FRotator MeshBaseRotation;
};
