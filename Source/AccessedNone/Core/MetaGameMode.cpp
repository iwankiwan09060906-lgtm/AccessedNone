#include "Core/MetaGameMode.h"
#include "Core/MetaPlayerController.h"
#include "Characters/MetaPlayerCharacter.h"

AMetaGameMode::AMetaGameMode()
{
	DefaultPawnClass = AMetaPlayerCharacter::StaticClass();
	PlayerControllerClass = AMetaPlayerController::StaticClass();
}
