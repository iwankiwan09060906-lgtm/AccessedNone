#include "Core/MetaGameMode.h"
#include "Core/MetaPlayerController.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Presentation/MetaPostProcessController.h"
#include "EngineUtils.h"
#include "Characters/MetaBossBase.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/BreakpointActor.h"
#include "Gimmick/EncounterTriggerBox.h"
#include "Meta/MetaSaveSubsystem.h"
#include "UI/MetaDeathScreenWidget.h"
#include "UI/MetaUI.h"
#include "TimerManager.h"

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
	// Encounters already won in this playthrough stay won (bosses gone).
	const UMetaSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UMetaSaveSubsystem>();
	for (TActorIterator<AEncounterTriggerBox> It(GetWorld()); It; ++It)
	{
		if (Saves->IsEncounterDefeated(It->EncounterId))
		{
			for (AMetaBossBase* Boss : It->Bosses)
			{
				if (Boss) { Boss->Destroy(); }
			}
			It->Destroy();
		}
	}
	Super::StartPlay();
}

void AMetaGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	StartTransform = Pawn->GetActorTransform();
	const FName Pending = GetGameInstance()->GetSubsystem<UMetaSaveSubsystem>()->ConsumePendingBreakpoint();
	for (TActorIterator<ABreakpointActor> It(GetWorld()); It && !Pending.IsNone(); ++It)
	{
		if (It->BreakpointId == Pending)
		{
			ActiveBreakpoint = *It;
			const FTransform At = It->GetRespawnTransform();
			Pawn->SetActorLocationAndRotation(At.GetLocation(), At.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
			NewPlayer->SetControlRotation(At.Rotator());
		}
	}
}

void AMetaGameMode::HandlePlayerDeath(AMetaPlayerCharacter* Player)
{
	if (!bRespawnEnabled || !Player)
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		DeathScreen = CreateWidget<UUserWidget>(PC, MetaUI::WidgetClass<UMetaDeathScreenWidget>(TEXT("WBP_DeathScreen")));
		DeathScreen->AddToViewport(15);
	}
	const float Delay = FMath::Min(UAccessedNoneSettings::Get().RespawnMaxSeconds - 0.5f, 2.5f);
	GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &AMetaGameMode::Respawn, TWeakObjectPtr<AMetaPlayerCharacter>(Player)), Delay, false);
}

void AMetaGameMode::Respawn(TWeakObjectPtr<AMetaPlayerCharacter> Player)
{
	if (DeathScreen)
	{
		DeathScreen->RemoveFromParent();
		DeathScreen = nullptr;
	}
	if (Player.IsValid())
	{
		const FTransform At = ActiveBreakpoint ? ActiveBreakpoint->GetRespawnTransform() : StartTransform;
		Player->Revive(At);
		if (AController* C = Player->GetController()) { C->SetControlRotation(At.Rotator()); }
	}
}
