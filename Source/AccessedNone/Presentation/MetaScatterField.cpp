#include "Presentation/MetaScatterField.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

AMetaScatterField::AMetaScatterField()
{
	Instances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	RootComponent = Instances;
}

void AMetaScatterField::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void AMetaScatterField::BeginPlay()
{
	Super::BeginPlay();
	if (Instances->GetInstanceCount() == 0)
	{
		Rebuild(); // properties set from Python never ran construction
	}
}

void AMetaScatterField::Rebuild()
{
	Instances->ClearInstances();
	Instances->SetStaticMesh(Mesh);
	Instances->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (!Mesh)
	{
		return;
	}
	FRandomStream Rng(Seed);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Count);
	for (int32 Tries = 0; Transforms.Num() < Count && Tries < Count * 10; ++Tries)
	{
		const float R = FMath::Sqrt(Rng.FRandRange(FMath::Square(InnerRadius / Radius), 1.f)) * Radius; // uniform over the ring
		const float A = Rng.FRandRange(0.f, UE_TWO_PI);
		if (FMath::Abs(R * FMath::Sin(A)) < CorridorHalfWidth)
		{
			continue;
		}
		Transforms.Emplace(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.f),
			FVector(Rng.FRandRange(ScaleRange.X, ScaleRange.Y)));
	}
	Instances->AddInstances(Transforms, false);
}
