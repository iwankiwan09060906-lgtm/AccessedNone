#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/MetaInteractable.h"
#include "BreakpointActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Checkpoint "Breakpoint" (spec 6.2): a red debugger breakpoint orb. E = full HP and flasks, save, respawn point.
 * Never saves during an encounter.
 */
UCLASS()
class ACCESSEDNONE_API ABreakpointActor : public AActor, public IMetaInteractable
{
	GENERATED_BODY()

public:
	ABreakpointActor();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Breakpoint") FName BreakpointId;

	/** Where the player reappears (in front of the orb, facing away from it). */
	FTransform GetRespawnTransform() const;

	virtual FText GetInteractPrompt() const override;
	virtual void Interact(AMetaPlayerCharacter* Player) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Orb;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	float BaseZ = 0.f;
};
