#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaBossBase.h"
#include "AssassinBoss.generated.h"

class UPointLightComponent;

/**
 * E1 "어둠 속의 암살자" (spec 5 E1): stealthed (mesh hidden, Gimmick.Stealth) and invulnerable, ambushes with an
 * instant kill. Only its eyes glint now and then (more often from hint tier 1). Gamma ≥ 0.95 reveals it during the
 * whiteout; once vulnerable it is stunned for StunSeconds. Data (mesh, patterns, restore) lives in BP_Assassin / DA_Boss_Assassin.
 */
UCLASS()
class ACCESSEDNONE_API AAssassinBoss : public AMetaBossBase
{
	GENERATED_BODY()

public:
	AAssassinBoss();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHintTier(int32 Tier) override { HintTier = Tier; }

	UPROPERTY(EditDefaultsOnly, Category="Assassin") float StunSeconds = 6.f;
	UPROPERTY(VisibleAnywhere, Category="Assassin") TObjectPtr<UPointLightComponent> EyeGlint;

	UFUNCTION(BlueprintPure, Category="Assassin") bool IsStealthed() const { return bStealthed; }

protected:
	virtual void BeginPlay() override;
	virtual void HandleGimmickStateChanged(EGimmickState Old, EGimmickState New) override;

private:
	void SetStealthed(bool bInStealthed);
	void EndStun();

	FTimerHandle StunTimer;
	float NextGlint = 0.f;
	float GlintEnd = 0.f;
	int32 HintTier = 0;
	bool bStealthed = false;
};
