#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "EncounterTriggerBox.generated.h"

class AMetaBossBase;

/** Place at a boss room entrance. Starts the encounter when the player walks in (spec 8.7). */
UCLASS()
class ACCESSEDNONE_API AEncounterTriggerBox : public ATriggerBox
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter") FName EncounterId;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Encounter") TArray<TObjectPtr<AMetaBossBase>> Bosses;

	/** Starts the encounter for all listed bosses (what entering the volume does). */
	UFUNCTION(BlueprintCallable, Category="Encounter") void StartEncounter();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION() void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);
};
