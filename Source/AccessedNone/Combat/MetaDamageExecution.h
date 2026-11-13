#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MetaDamageExecution.generated.h"

/** Spec 3.2 steps 1–4. Outputs IncomingRawDamage then IncomingDamage; the attribute set does steps 5–6. */
UCLASS()
class ACCESSEDNONE_API UMetaDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UMetaDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
