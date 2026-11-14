#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MetaGameMode.generated.h"

UCLASS()
class ACCESSEDNONE_API AMetaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMetaGameMode();
	/** Ensures the level has an AMetaPostProcessController (spec 8.9: one per level). */
	virtual void StartPlay() override;
};
