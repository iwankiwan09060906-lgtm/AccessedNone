#include "Gimmick/EncounterTriggerBox.h"
#include "Characters/MetaBossBase.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Meta/EncounterSubsystem.h"

void AEncounterTriggerBox::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &AEncounterTriggerBox::HandleOverlap);
}

void AEncounterTriggerBox::HandleOverlap(AActor* /*OverlappedActor*/, AActor* OtherActor)
{
	if (Cast<AMetaPlayerCharacter>(OtherActor))
	{
		StartEncounter();
	}
}

void AEncounterTriggerBox::StartEncounter()
{
	UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>();
	for (AMetaBossBase* Boss : Bosses)
	{
		Encounters->BeginEncounter(EncounterId, Boss);
	}
}
