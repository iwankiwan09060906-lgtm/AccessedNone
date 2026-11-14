#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MetaInteractable.generated.h"

class AMetaPlayerCharacter;

UINTERFACE(MinimalAPI)
class UMetaInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Things the player uses with E (spec 2.1): breakpoints, inscriptions, later dialogue and finishers. */
class ACCESSEDNONE_API IMetaInteractable
{
	GENERATED_BODY()

public:
	/** Shown above the HUD while the player is in range. Empty = not usable right now. */
	virtual FText GetInteractPrompt() const = 0;
	virtual void Interact(AMetaPlayerCharacter* Player) = 0;
};
