#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaBossBase.h"
#include "ThornKnightBoss.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * E2 "가시 갑옷의 기사" (spec 5 E2): high-poly thorn armor (Spike_Hitbox_* cones), "Triangles: 48,000,000" over its head,
 * a fake frame drop when the fight starts, 99 % reduction + 300 % reflect (data). Mesh quality 0 → the thorns dissolve in
 * 0.6 s, the body drops to its lowest LOD (the "SK_ThornKnight_Low" swap) and all spike collision turns off.
 */
UCLASS()
class ACCESSEDNONE_API AThornKnightBoss : public AMetaBossBase
{
	GENERATED_BODY()

public:
	AThornKnightBoss();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHintTier(int32 Tier) override { HintTier = Tier; }

	UFUNCTION(BlueprintPure, Category="ThornKnight") bool IsLowPoly() const { return bLowPoly; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetSpikes() const { return Spikes; }

	UPROPERTY(EditDefaultsOnly, Category="ThornKnight") float DissolveSeconds = 0.6f;
	UPROPERTY(EditDefaultsOnly, Category="ThornKnight") float FrameDropSeconds = 1.5f;
	UPROPERTY(EditDefaultsOnly, Category="ThornKnight") int32 FrameDropFPS = 9;

protected:
	virtual void BeginPlay() override;
	virtual void HandleGimmickStateChanged(EGimmickState Old, EGimmickState New) override;

private:
	void SetHighPoly();
	void FinishLowPoly();
	void PlayFrameDrop();

	UPROPERTY(VisibleAnywhere, Category="ThornKnight") TArray<TObjectPtr<UStaticMeshComponent>> Spikes;
	UPROPERTY(VisibleAnywhere, Category="ThornKnight") TObjectPtr<UTextRenderComponent> TrisText;
	TArray<FVector> SpikeScales;
	float DissolveElapsed = -1.f;
	int32 HintTier = 0;
	bool bLowPoly = false;
};
