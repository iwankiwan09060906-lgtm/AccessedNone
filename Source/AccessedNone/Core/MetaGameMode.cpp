#include "Core/MetaGameMode.h"
#include "Core/MetaPlayerController.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Presentation/MetaPostProcessController.h"
#include "EngineUtils.h"

AMetaGameMode::AMetaGameMode()
{
	DefaultPawnClass = AMetaPlayerCharacter::StaticClass();
	PlayerControllerClass = AMetaPlayerController::StaticClass();
}

void AMetaGameMode::StartPlay()
{
	if (!TActorIterator<AMetaPostProcessController>(GetWorld()))
	{
		GetWorld()->SpawnActor<AMetaPostProcessController>();
	}
	Super::StartPlay();
}
