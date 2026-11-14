#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/MetaInteractable.h"
#include "MetaInscription.generated.h"

/** Readable stone (E1_STELE etc.): E shows its ST_Game text on screen for a while. Mesh is set per level. */
UCLASS()
class ACCESSEDNONE_API AMetaInscription : public AActor, public IMetaInteractable
{
	GENERATED_BODY()

public:
	AMetaInscription();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inscription") FName TextKey;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inscription") TObjectPtr<UStaticMeshComponent> Mesh;

	virtual FText GetInteractPrompt() const override;
	virtual void Interact(AMetaPlayerCharacter* Player) override;
};
