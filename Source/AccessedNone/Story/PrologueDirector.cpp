#include "Story/PrologueDirector.h"
#include "Characters/DefenseEntity.h"
#include "Characters/MetaBossBase.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaGameplayAbility.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaLevels.h"
#include "Core/MetaLog.h"
#include "Core/MetaPlayerController.h"
#include "Core/MetaText.h"
#include "Gimmick/EncounterTriggerBox.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/HintSubsystem.h"
#include "UI/MetaDeathScreenWidget.h"
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
	constexpr float SummonFXScale = 3.f;

	// 4.1 clear
	constexpr float ClearHoldSeconds = 4.f;
	const FLinearColor HolyTint(MetaUI::Gold.R, MetaUI::Gold.G, MetaUI::Gold.B, 0.12f);
	// 4.2 timeline
	constexpr float TapeStopSeconds = 2.f;
	constexpr float TapeStopMinPitch = 0.3f;
	constexpr float GlitchAt = 2.f;
	constexpr float CrackAt = 2.5f;
	constexpr float CrackOpenSeconds = 4.f;
	constexpr float EntitiesAt = 6.f;
	constexpr float CrackPhaseSeconds = 11.f;
	constexpr float CrackDistance = 10000.f;
	constexpr float CrackHeight = 3800.f;
	constexpr float EntitySpawnHeight = 2800.f;
	constexpr float EntityMinRadius = 500.f;
	constexpr float EntityMaxRadius = 2600.f;
	// 4.3 log lines, one every LogInterval
	const TCHAR* LogKeys[] = { TEXT("OP_P3_LOG_01"), TEXT("OP_P3_LOG_02"), TEXT("OP_P3_DLG_A"), TEXT("OP_P3_DLG_B"), TEXT("OP_P3_LOG_03") };
	constexpr float LogInterval = 1.8f;
	constexpr float LogsPhaseSeconds = 10.f;
	// 4.4 forced defeat: the blast kills at KillAt regardless of input (spec: ~5 s)
	constexpr float ExplodeAt = 1.0f;
	constexpr float KillAt = 1.3f;
	constexpr int32 SideBlasts = 10;
	constexpr float BlastLaunch = 2600.f;
	constexpr double FlashSeconds = 1.6; // single fade, no strobing (spec 12.3)
	const FLinearColor FlashColor(1.f, 0.85f, 0.6f, 0.9f);
	// 4.5
	constexpr float DeathScreenAt = 0.8f;
	constexpr float DebuggerAt = 2.8f;
	constexpr float ErrorWindowAt = 5.f;
	constexpr int32 DeathScreenZOrder = 15;
	constexpr int32 ErrorWindowZOrder = 20;
}

APrologueDirector::APrologueDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
	MusicComponent->bAutoActivate = false;
	MusicComponent->bIsUISound = true; // 2D
	RootComponent = MusicComponent;
	Music = FSoftObjectPath(TEXT("/Game/Audio/SW_BGM_Prologue.SW_BGM_Prologue"));
	ExplosionSound = FSoftObjectPath(TEXT("/Game/Audio/SW_Explosion.SW_Explosion"));
	SummonFX = FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Ability/Summon/P_Summon_Portal.P_Summon_Portal"));
	ExplosionFX = {
		TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Cines/cinema03/P_CIN_IB_Explosion.P_CIN_IB_Explosion"))),
		TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Cines/cinema03/P_CIN_IB_Explosion_02.P_CIN_IB_Explosion_02"))),
		TSoftObjectPtr<UParticleSystem>(FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Skill_RockBurst/P_RBurst_Fire_Eruption_01.P_RBurst_Fire_Eruption_01"))) };
	SideBlastFX = FSoftObjectPath(TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Gruntling/Bomber/P_FireBombExplosion.P_FireBombExplosion"));
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
		// The fake boss waits unseen in the dungeon until its encounter summons it.
		if (const AEncounterTriggerBox* Trigger = Cast<AEncounterTriggerBox>(*It); Trigger && Trigger->EncounterId == FightEncounterId)
		{
			for (AMetaBossBase* Boss : Trigger->Bosses)
			{
				if (Boss)
				{
					Boss->SetActorHiddenInGame(true);
					SummonedBosses.Add(Boss);
				}
			}
		}
	}
	if (UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>())
	{
		Encounters->OnEncounterBegan.AddDynamic(this, &APrologueDirector::HandleEncounterBegan);
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
			DeadSince = GetWorld()->GetTimeSeconds(); // died to the fake enemies: restart the prologue
		}
		else if (GetWorld()->GetTimeSeconds() - DeadSince > RestartAfterDeathSeconds)
		{
			UGameplayStatics::OpenLevel(this, MetaLevels::Prologue);
			Phase = EProloguePhase::Done;
		}
	}

	if (FlashStart >= 0.0)
	{
		const float A = 1.f - static_cast<float>((GetWorld()->GetTimeSeconds() - FlashStart) / FlashSeconds);
		if (UMetaHUDWidget* HUD = GetHUD()) { HUD->SetScreenTint(FlashColor * FLinearColor(1, 1, 1, FMath::Max(A, 0.f))); }
		if (A <= 0.f) { FlashStart = -1.0; }
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
	TickEntityRain();

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

void APrologueDirector::HandleEncounterBegan(FName EncounterId)
{
	if (EncounterId != FightEncounterId)
	{
		return;
	}
	UParticleSystem* FX = SummonFX.LoadSynchronous();
	for (AMetaBossBase* Boss : SummonedBosses)
	{
		Boss->SetActorHiddenInGame(false);
		if (FX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, FX, Boss->GetActorLocation() - FVector(0, 0, 90.f), FRotator::ZeroRotator, FVector(SummonFXScale));
		}
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
	if (HolyLight)
	{
		if (const AMetaPlayerCharacter* Player = GetPlayer(); Player && Passed(0.f))
		{
			HolyLight->GetOwner()->SetActorLocation(Player->GetActorLocation() + FVector(0, 0, 600.f)); // light where the boss fell
		}
		HolyLight->SetIntensity(HolyLightIntensity * Alpha);
	}
	if (UMetaHUDWidget* HUD = GetHUD()) { HUD->SetScreenTint(HolyTint * FLinearColor(1, 1, 1, Alpha)); }
	if (PhaseTime >= ClearHoldSeconds)
	{
		EnterPhase(EProloguePhase::Crack);
	}
}

// --- 4.2 ---------------------------------------------------------------------

void APrologueDirector::PlaceCrackInView()
{
	const AMetaPlayerCharacter* Player = GetPlayer();
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!CrackActor || !Player || !PC)
	{
		return;
	}
	const float Yaw = PC->GetControlRotation().Yaw;
	const FVector Dir = FRotator(0.f, Yaw, 0.f).Vector();
	CrackActor->SetActorLocation(Player->GetActorLocation() + Dir * CrackDistance + FVector(0, 0, CrackHeight));
	CrackActor->SetActorRotation(FRotator(0.f, Yaw - 90.f, 90.f)); // engine plane faces +Z; stand it up facing the player
}

void APrologueDirector::TickCrack()
{
	UMetaHUDWidget* HUD = GetHUD();

	if (Passed(0.f))
	{
		PlaceCrackInView();
	}

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
	}

	if (Passed(EntitiesAt))
	{
		NextEntityTime = GetWorld()->GetTimeSeconds();
	}
	if (PhaseTime >= CrackPhaseSeconds)
	{
		EnterPhase(EProloguePhase::Logs);
	}
}

void APrologueDirector::TickEntityRain()
{
	const AMetaPlayerCharacter* Player = GetPlayer();
	if (NextEntityTime < 0.0 || Entities.Num() >= EntityCount || !Player || GetWorld()->GetTimeSeconds() < NextEntityTime)
	{
		return;
	}
	NextEntityTime = GetWorld()->GetTimeSeconds() + EntitySpawnInterval;
	const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
	const float Dist = FMath::FRandRange(EntityMinRadius, EntityMaxRadius);
	const FVector Center = Player->GetActorLocation();
	const FVector Loc = Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, EntitySpawnHeight + FMath::FRandRange(0.f, 1500.f));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADefenseEntity* E = GetWorld()->SpawnActor<ADefenseEntity>(ADefenseEntity::StaticClass(), Loc, (Center - Loc).GetSafeNormal2D().Rotation(), Params))
	{
		Entities.Add(E);
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
	if (Passed(ExplodeAt))
	{
		Explode();
	}
	if (Passed(KillAt))
	{
		KillPlayer();
	}
}

void APrologueDirector::Explode()
{
	const AMetaPlayerCharacter* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const FVector P = Player->GetActorLocation();
	for (const TSoftObjectPtr<UParticleSystem>& Soft : ExplosionFX)
	{
		if (UParticleSystem* FX = Soft.LoadSynchronous())
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, FX, P, FRotator::ZeroRotator, FVector(ExplosionScale));
		}
	}
	if (UParticleSystem* Side = SideBlastFX.LoadSynchronous())
	{
		for (int32 i = 0; i < SideBlasts; ++i)
		{
			const FVector Offset = FRotator(0.f, 360.f * i / SideBlasts, 0.f).Vector() * FMath::FRandRange(500.f, 1800.f);
			UGameplayStatics::SpawnEmitterAtLocation(this, Side, P + Offset, FRotator::ZeroRotator, FVector(FMath::FRandRange(3.f, 6.f)));
		}
	}
	if (USoundBase* Sound = ExplosionSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
	for (ADefenseEntity* E : Entities)
	{
		if (IsValid(E)) { E->BlastAway(P, BlastLaunch); }
	}
	FlashStart = GetWorld()->GetTimeSeconds();
	NextEntityTime = -1.0;
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
	UAbilitySystemComponent* Source = !Entities.IsEmpty() && IsValid(Entities[0]) ? Entities[0]->GetAbilitySystemComponent() : Player->GetAbilitySystemComponent();
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
	AMetaPlayerController* PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (Passed(0.f))
	{
		if (HUD)
		{
			HUD->SetStatusVisible(false);
			HUD->ClearLog();
			HUD->ShowBanner(FText::GetEmpty(), MetaUI::Error);
		}
	}
	if (Passed(DeathScreenAt) && PC)
	{
		DeathScreen = CreateWidget<UMetaDeathScreenWidget>(PC, MetaUI::WidgetClass<UMetaDeathScreenWidget>(TEXT("WBP_DeathScreen")));
		DeathScreen->AddToViewport(DeathScreenZOrder);
	}
	if (Passed(DebuggerAt))
	{
		GetGameInstance()->GetSubsystem<UHintSubsystem>()->PushDebuggerMessage(MetaText::Get(TEXT("OP_P5_DBG_01")));
	}
	if (Passed(ErrorWindowAt) && PC)
	{
		ErrorWindow = CreateWidget<UMetaErrorWindowWidget>(PC, MetaUI::WidgetClass<UMetaErrorWindowWidget>(TEXT("WBP_ErrorWindow")));
		ErrorWindow->OnFinished.AddDynamic(this, &APrologueDirector::HandleErrorWindowFinished);
		ErrorWindow->AddToViewport(ErrorWindowZOrder);
		PC->SetUIManipulation(true);
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
