#include "Combat/MeleeHitboxComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarDebugHitbox(TEXT("an.Debug.Hitbox"), 0, TEXT("1 = draw melee hitboxes"));

TArray<AActor*> UMeleeHitboxComponent::QueryHits(const FVector& Extent, const FVector& LocalOffset) const
{
	TArray<AActor*> Hits;
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return Hits;
	}

	const FTransform& Xf = Owner->GetActorTransform();
	const FVector Center = Xf.TransformPosition(LocalOffset);
	const FQuat Rot = Xf.GetRotation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MeleeHitbox), false, Owner);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, Rot, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeBox(Extent), Params);
	for (const FOverlapResult& O : Overlaps)
	{
		if (AActor* A = O.GetActor())
		{
			Hits.AddUnique(A);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (CVarDebugHitbox.GetValueOnGameThread() != 0)
	{
		DrawDebugBox(World, Center, Extent, Rot, Hits.IsEmpty() ? FColor::Yellow : FColor::Red, false, 0.5f);
	}
#endif
	return Hits;
}
