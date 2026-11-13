#pragma once

#include "CoreMinimal.h"
#include "Characters/MetaBossBase.h"
#include "DevTestBoss.generated.h"

/** S1-T08 sandbox boss: breaks when Gamma ≥ 0.95, Gamma forced to 0.2 on entry. Data (effects, patterns) comes from BP_TestBoss/DA_Boss_Test. */
UCLASS()
class ACCESSEDNONE_API ADevTestBoss : public AMetaBossBase
{
	GENERATED_BODY()

public:
	ADevTestBoss();
};
