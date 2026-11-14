#include "Story/PrologueDirector.h"
#include "Characters/DefenseEntity.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaGameplayAbility.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaLevels.h"
#include "Core/MetaLog.h"
#include "Core/MetaPlayerController.h"
#include "Core/MetaText.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "UI/MetaBossBarOverflowWidget.h"
#include "UI/MetaErrorWindowWidget.h"
#include "UI/MetaHUDWidget.h"
#include "UI/MetaUI.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

namespace
{
	const TCHAR* TutorialKeys[] = { TEXT("OP_P1_TUT_MOVE"), TEXT("OP_P1_TUT_DODGE"), TEXT("OP_P1_TUT_LIGHT"), TEXT("OP_P1_TUT_HEAVY"), TEXT("OP_P1_TUT_HEAL") };
	constexpr int32 TutorialCount = UE_ARRAY_COUNT(TutorialKeys);
	constexpr float MoveTutorialSeconds = 1.f;
	constexpr float RestartAfterDeathSeconds = 3.f; // spec 6.2 respawn ≤ 3 s (no breakpoint yet in the prologue)

	// 4.1 clear
	constexpr float ClearHoldSeconds = 4.f;
	const FLinearColor HolyTint(MetaUI::Gold.R, MetaUI::Gold.G, MetaUI::Gold.B, 0.12f);
	// 4.2 timeline
	constexpr float TapeStopSeconds = 2.f;
	constexpr float TapeStopMinPitch = 0.3f;
	constexpr float GlitchAt = 2.f;
	constexpr float CrackAt = 2.5f;
	constexpr float CrackOpenSeconds = 3.f;
	constexpr float LightningInterval = 0.7f;
	constexpr float EntitiesAt = 8.f;
	constexpr float CrackPhaseSeconds = 11.f;
	constexpr float EntitySpawnHeight = 2500.f;
	// 4.3 log lines, one every LogInterval
	const TCHAR* LogKeys[] = { TEXT("OP_P3_LOG_01"), TEXT("OP_P3_LOG_02"), TEXT("OP_P3_DLG_A"), TEXT("OP_P3_DLG_B"), TEXT("OP_P3_LOG_03") };
	constexpr float LogInterval = 1.8f;
	constexpr float LogsPhaseSeconds = 10.f;
	// 4.4 forced defeat: death is guaranteed at KillAt regardless of input (spec: ~5 s)
	constexpr float ExecutorStartDistance = 1100.f;
	constexpr float ExecutorStrikeDistance = 140.f;
	constexpr float SlideSeconds = 2.4f;
	constexpr float SwingAt = 2.6f;
	constexpr float SlashAt = 2.9f;
	constexpr int32 SlashCount = 24;
	constexpr float KillAt = 3.4f;
	// 4.5
	constexpr float DeathTextAt = 1.f;
	constexpr float DebuggerAt = 2.5f;
	constexpr float ErrorWindowAt = 5.f;
	constexpr int32 ErrorWindowZOrder = 20;
	constexpr int32 OverflowBarZOrder = 4;
}

APrologueDirector::APrologueDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
	MusicComponent->bAutoActivate = false;
	MusicComponent->bIsUISound = true; // 2D
	RootComponent = MusicComponent;
	Music = FSoftObjectPath(TEXT("/Game/Audio/SW_BGM_Prologue.SW_BGM_Prologue"));
	LightningFX = FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Combat_Lightning/P_DOT_Lightning_01.P_DOT_Lightning_01"));
	SlashFX = FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Mobile/Fire/combat/P_HeroSwipe_Fire.P_HeroSwipe_Fire"));
}

void APrologueDirector::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("SkyCrack")))
		{
			CrackActor = *It;
			if (UStaticMeshComponent* Mesh = It->FindComponentByClass<UStaticMeshComponent>())
			{
				CrackMID = Mesh->CreateDynamicMaterialInstance(0);
				CrackMID->SetScalarParameterValue(TEXT("CrackProgress"), 0.f);
			}
		}
		if (It->ActorHasTag(TEXT("HolyLight")))
		{
			HolyLight = It->FindComponentByClass<ULightComponent>();
			if (HolyLight) { HolyLight->SetIntensity(0.f); }
		}
	}
	if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>())
	{
		Encounters->OnEncounterEnded.AddDynamic(this, &APrologueDirector::HandleEncounterEnded);
	}
	if (USoundBase* Sound = Music.LoadSynchronous())
	{
		MusicComponent->SetSound(Sound);
		MusicComponent->Play();
	}
	EnterPhase(EProloguePhase::Fight);
}

AMetaPlayerCharacter* APrologueDirector::GetPlayer() const
{
	return Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

UMetaHUDWidget* APrologueDirector::GetHUD() const
{
	const AMetaPlayerController* PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	return PC ? PC->GetHUDWidget() : nullptr;
}

void APrologueDirector::EnterPhase(EProloguePhase NewPhase)
{
	UE_LOG(LogAccessedNone, Log, TEXT("Prologue: %s -> %s"), *UEnum::GetValueAsString(Phase), *UEnum::GetValueAsString(NewPhase));
	Phase = NewPhase;
	PhaseTime = 0.f;
	PrevPhaseTime = -1.f;
	bPhaseEntered = true;
}

void APrologueDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AMetaPlayerCharacter* Player = GetPlayer();
	if (Player && Player->IsDead() && Phase != EProloguePhase::Dead && Phase != EProloguePhase::Done)
	{
		if (Phase == EProloguePhase::Execution)
		{
			EnterPhase(EProloguePhase::Dead);
		}
		else if (DeadSince < 0.f)
		{
			DeadSince = GetWorld()->GetTimeSeconds(); // died to the fake boss: restart the prologue
		}
		else if (GetWorld()->GetTimeSeconds() - DeadSince > RestartAfterDeathSeconds)
		{
			UGameplayStatics::OpenLevel(this, MetaLevels::Prologue);
			Phase = EProloguePhase::Done;
		}
	}

	switch (bPhaseEntered ? EProloguePhase::Done : Phase) // just entered above: start next tick
	{
	case EProloguePhase::Fight:     TickFight(DeltaSeconds); break;
	case EProloguePhase::Clear:     TickClear(); break;
	case EProloguePhase::Crack:     TickCrack(); break;
	case EProloguePhase::Logs:      TickLogs(); break;
	case EProloguePhase::Execution: TickExecution(); break;
	case EProloguePhase::Dead:      TickDead(); break;
	default: break;
	}
	// A phase entered during this tick starts its clock next tick, so its Passed(0) still fires.
	if (bPhaseEntered)
	{
		bPhaseEntered = false;
		return;
	}
	PrevPhaseTime = PhaseTime;
	PhaseTime += DeltaSeconds;
}

// --- 4.1 ---------------------------------------------------------------------

void APrologueDirector::SetTutorialStep(int32 Step)
{
	TutorialStep = Step;
	TutorialStepTime = 0.f;
	if (UMetaHUDWidget* HUD = GetHUD())
	{
		HUD->ShowTutorial(Step >= 0 && Step < TutorialCount ? MetaText::Get(TutorialKeys[Step]) : FText::GetEmpty());
	}
}

void APrologueDirector::HandleAbilityActivated(UGameplayAbility* Ability)
{
	const UClass* Expected[] = { nullptr, UGA_Dodge::StaticClass(), UGA_LightAttack::StaticClass(), UGA_HeavyAttack::StaticClass(), UGA_Heal::StaticClass() };
	if (Phase == EProloguePhase::Fight && TutorialStep > 0 && TutorialStep < TutorialCount && Ability && Ability->GetClass()->IsChildOf(Expected[TutorialStep]))
	{
		SetTutorialStep(TutorialStep + 1);
	}
}

void APrologueDirector::TickFight(float Dt)
{
	AMetaPlayerCharacter* Player = GetPlayer();
	if (!Player || !GetHUD())
	{
		return;
	}
	if (TutorialStep < 0)
	{
		Player->GetAbilitySystemComponent()->AbilityActivatedCallbacks.AddUObject(this, &APrologueDirector::HandleAbilityActivated);
		SetTutorialStep(0);
		return;
	}
	if (TutorialStep >= TutorialCount)
	{
		return;
	}
	TutorialStepTime += Dt;
	if (TutorialStep == 0 && Player->GetVelocity().SizeSquared2D() > 100.f)
	{
		MoveTime += Dt;
	}
	if ((TutorialStep == 0 && MoveTime >= MoveTutorialSeconds) || TutorialStepTime > TutorialStepTimeout)
	{
		SetTutorialStep(TutorialStep + 1);
	}
}

void APrologueDirector::HandleEncounterEnded(FName EncounterId)
{
	if (Phase == EProloguePhase::Fight && EncounterId == FightEncounterId)
	{
		SetTutorialStep(TutorialCount);
		if (UMetaHUDWidget* HUD = GetHUD())
		{
			HUD->ShowBanner(MetaText::Get(TEXT("UI_DEMON_VANQUISHED")), MetaUI::Gold);
		}
		EnterPhase(EProloguePhase::Clear);
	}
}

void APrologueDirector::TickClear()
{
	const float Alpha = FMath::Clamp(PhaseTime / 2.f, 0.f, 1.f);
	if (HolyLight) { HolyLight->SetIntensity(HolyLightIntensity * Alpha); }
	if (UMetaHUDWidget* HUD = GetHUD()) { HUD->SetScreenTint(HolyTint * FLinearColor(1, 1, 1, Alpha)); }
	if (PhaseTime >= ClearHoldSeconds)
	{
		EnterPhase(EProloguePhase::Crack);
	}
}

// --- 4.2 ---------------------------------------------------------------------

void APrologueDirector::TickCrack()
{
	UMetaHUDWidget* HUD = GetHUD();

	// Tape stop: pitch (= resample speed) 1.0 → 0.3 over 2 s, then stop.
	if (MusicComponent->IsPlaying())
	{
		const float A = FMath::Clamp(PhaseTime / TapeStopSeconds, 0.f, 1.f);
		MusicComponent->SetPitchMultiplier(FMath::Lerp(1.f, TapeStopMinPitch, A));
		if (A >= 1.f) { MusicComponent->Stop(); }
	}

	if (Passed(GlitchAt) && HUD)
	{
		HUD->GlitchBanner(MetaText::Get(TEXT("UI_DEMON_VANQUISHED_GLITCH")));
	}

	if (PhaseTime >= CrackAt)
	{
		const float Open = FMath::Clamp((PhaseTime - CrackAt) / CrackOpenSeconds, 0.f, 1.f);
		if (CrackMID) { CrackMID->SetScalarParameterValue(TEXT("CrackProgress"), Open); }
		if (HolyLight) { HolyLight->SetIntensity(HolyLightIntensity * (1.f - Open)); }
		if (HUD) { HUD->SetScreenTint(HolyTint * FLinearColor(1, 1, 1, 1.f - Open)); }

		if (PhaseTime >= NextLightning && CrackMID)
		{
			NextLightning = PhaseTime + LightningInterval;
			if (UParticleSystem* FX = LightningFX.LoadSynchronous())
			{
				const FVector Base = CrackActor ? CrackActor->GetActorLocation() : GetActorLocation() + FVector(0, 0, 3000);
				UGameplayStatics::SpawnEmitterAtLocation(this, FX, Base + FMath::VRand() * FVector(1500.f, 1500.f, 600.f), FRotator::ZeroRotator, FVector(6.f));
			}
		}
	}

	if (Passed(EntitiesAt))
	{
		SpawnEntities();
	}
	if (PhaseTime >= CrackPhaseSeconds)
	{
		EnterPhase(EProloguePhase::Logs);
	}
}

void APrologueDirector::SpawnEntities()
{
	const AMetaPlayerCharacter* Player = GetPlayer();
	const FVector Center = Player ? Player->GetActorLocation() : GetActorLocation();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < EntityCount; ++i)
	{
		const float Angle = UE_TWO_PI * i / EntityCount + FMath::FRandRange(-0.3f, 0.3f);
		const float Dist = FMath::FRandRange(800.f, 1600.f);
		const FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, EntitySpawnHeight + FMath::FRandRange(0.f, 800.f));
		const FRotator Face = (Center - Loc).GetSafeNormal2D().Rotation();
		if (ADefenseEntity* E = GetWorld()->SpawnActor<ADefenseEntity>(ADefenseEntity::StaticClass(), Loc, Face, Params))
		{
			E->SetExplode(FMath::FRandRange(0.3f, 1.f));
			Entities.Add(E);
		}
	}
}

// --- 4.3 ---------------------------------------------------------------------

void APrologueDirector::TickLogs()
{
	UMetaHUDWidget* HUD = GetHUD();
	if (Passed(0.f) && HUD)
	{
		HUD->ShowBanner(FText::GetEmpty(), MetaUI::Gold);
	}
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(LogKeys)); ++i)
	{
		if (Passed(0.5f + i * LogInterval) && HUD)
		{
			const bool bSystem = FString(LogKeys[i]).Contains(TEXT("_LOG_"));
			HUD->PushLogLine(MetaText::Get(LogKeys[i]), bSystem ? MetaUI::Error : FLinearColor(0.85f, 0.8f, 0.95f));
		}
	}
	if (PhaseTime >= LogsPhaseSeconds)
	{
		EnterPhase(EProloguePhase::Execution);
	}
}

// --- 4.4 ---------------------------------------------------------------------

void APrologueDirector::TickExecution()
{
	AMetaPlayerCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const FVector P = Player->GetActorLocation();
	const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();

	if (Passed(0.f))
	{
		Entities.RemoveAll([](const TObjectPtr<ADefenseEntity>& E) { return !IsValid(E); });
		if (Entities.IsEmpty())
		{
			SpawnEntities();
		}
		Executor = Entities[0];
		Executor->SetActorLocation(P + Forward * ExecutorStartDistance + FVector(0, 0, 50.f), false, nullptr, ETeleportType::TeleportPhysics);
		Executor->SlideTo(P + Forward * ExecutorStrikeDistance, SlideSeconds);
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			OverflowBar = CreateWidget<UMetaBossBarOverflowWidget>(PC, MetaUI::WidgetClass<UMetaBossBarOverflowWidget>(TEXT("WBP_BossBar_Overflow")));
			OverflowBar->AddToViewport(OverflowBarZOrder);
		}
	}
	if (Passed(SwingAt) && Executor)
	{
		// Wherever the player ran, the entity snaps next to them (the hit is scripted, spec 4.4).
		const FVector ToPlayer = (P - Executor->GetActorLocation()).GetSafeNormal2D();
		Executor->SetActorLocation(P - ToPlayer * ExecutorStrikeDistance, false, nullptr, ETeleportType::TeleportPhysics);
		Executor->SetActorRotation(ToPlayer.Rotation());
		Executor->ChoppySwing();
	}
	if (Passed(SlashAt))
	{
		if (UParticleSystem* FX = SlashFX.LoadSynchronous())
		{
			for (int32 i = 0; i < SlashCount; ++i)
			{
				UGameplayStatics::SpawnEmitterAtLocation(this, FX, P + FMath::VRand() * 250.f + FVector(0, 0, 60.f), FRotator(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(-90.f, 90.f)));
			}
		}
	}
	if (Passed(KillAt))
	{
		KillPlayer();
	}
}

void APrologueDirector::KillPlayer()
{
	AMetaPlayerCharacter* Player = GetPlayer();
	if (!Player || Player->IsDead())
	{
		return;
	}
	UMetaHUDWidget* HUD = GetHUD();
	if (HUD)
	{
		HUD->SetDamageNumbersEnabled(false); // the real number is 99,999; the scripted text shows -999,999,999
		HUD->ShowFloatingText(Player->GetActorLocation() + FVector(0, 0, 120), MetaText::Get(TEXT("OP_P4_DAMAGE")), MetaUI::Error, 44.f);
	}
	UAbilitySystemComponent* Source = Executor ? Executor->GetAbilitySystemComponent() : Player->GetAbilitySystemComponent();
	const FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, Source->MakeEffectContext());
	Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, Player->GetAbilitySystemComponent());
	if (HUD)
	{
		HUD->SetDamageNumbersEnabled(true);
	}
}

// --- 4.5 ---------------------------------------------------------------------

void APrologueDirector::TickDead()
{
	UMetaHUDWidget* HUD = GetHUD();
	if (Passed(0.f))
	{
		if (OverflowBar) { OverflowBar->RemoveFromParent(); }
		if (HUD)
		{
			HUD->SetStatusVisible(false);
			HUD->ClearLog();
			HUD->ShowBanner(FText::GetEmpty(), MetaUI::Error);
		}
	}
	if (Passed(DeathTextAt) && HUD)
	{
		HUD->ShowBanner(MetaText::Get(TEXT("UI_DEATH")), MetaUI::Error, 72.f);
	}
	if (Passed(DebuggerAt))
	{
		GetGameInstance()->GetSubsystem<UHintSubsystem>()->PushDebuggerMessage(MetaText::Get(TEXT("OP_P5_DBG_01")));
	}
	if (Passed(ErrorWindowAt))
	{
		if (AMetaPlayerController* PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		{
			ErrorWindow = CreateWidget<UMetaErrorWindowWidget>(PC, MetaUI::WidgetClass<UMetaErrorWindowWidget>(TEXT("WBP_ErrorWindow")));
			ErrorWindow->OnFinished.AddDynamic(this, &APrologueDirector::HandleErrorWindowFinished);
			ErrorWindow->AddToViewport(ErrorWindowZOrder);
			PC->SetUIManipulation(true);
		}
	}
}

void APrologueDirector::HandleErrorWindowFinished()
{
	EnterPhase(EProloguePhase::Done);
	if (AMetaPlayerController* PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->SetUIManipulation(false);
	}
	UGameplayStatics::OpenLevel(this, MetaLevels::Stage1);
}
