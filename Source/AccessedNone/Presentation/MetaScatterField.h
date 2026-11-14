#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetaScatterField.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/** Level dressing: scatters Count instances of Mesh in a disc (flowers, grass, trees). Python sets the properties. */
UCLASS()
class ACCESSEDNONE_API AMetaScatterField : public AActor
{
	GENERATED_BODY()

public:
	AMetaScatterField();
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Category="Scatter") TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, Category="Scatter") int32 Count = 200;
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(Units="cm")) float Radius = 2000.f;
	/** Keeps the middle clear (arena). */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(Units="cm")) float InnerRadius = 0.f;
	/** Keeps a straight lane along local X clear (|Y| < this), e.g. a road through a forest. 0 = off. */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(Units="cm")) float CorridorHalfWidth = 0.f;
	UPROPERTY(EditAnywhere, Category="Scatter") FVector2D ScaleRange = FVector2D(0.8f, 1.2f);
	UPROPERTY(EditAnywhere, Category="Scatter") int32 Seed = 1;
	UPROPERTY(EditAnywhere, Category="Scatter") bool bCollision = false;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Scatter") void Rebuild();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;
};
