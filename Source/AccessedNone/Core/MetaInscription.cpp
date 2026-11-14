#include "Core/MetaInscription.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Core/MetaPlayerController.h"
#include "Core/MetaText.h"
#include "UI/MetaHUDWidget.h"
#include "UI/MetaUI.h"
#include "Components/StaticMeshComponent.h"

AMetaInscription::AMetaInscription()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

FText AMetaInscription::GetInteractPrompt() const
{
	return MetaText::Get(TEXT("UI_READ_PROMPT"));
}

void AMetaInscription::Interact(AMetaPlayerCharacter* Player)
{
	if (const AMetaPlayerController* PC = Player ? Cast<AMetaPlayerController>(Player->GetController()) : nullptr; PC && PC->GetHUDWidget())
	{
		PC->GetHUDWidget()->ShowToast(MetaText::Get(*TextKey.ToString()), 6.f, FLinearColor(0.9f, 0.86f, 0.76f));
	}
}
