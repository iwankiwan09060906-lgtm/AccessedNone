#include "Misc/AutomationTest.h"
#include "Animation/AnimInstance.h"
#include "Core/MetaGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Characters/MetaBossBase.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/BossPatternComponent.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayAbility.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaPlayerController.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Combat/MetaGameplayEffects.h"
#include "UnrealClient.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * S1 DoD, end-to-end in a running game:
 *   UnrealEditor-Cmd AccessedNone.uproject /Game/Maps/L_Dev_Sandbox -game -nullrhi -ExecCmds="Automation RunTests AccessedNone.Sandbox; Quit"
 * Client context only, so it never runs in the editor-context suite.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxBossFlowTest, "AccessedNone.Sandbox.BossFlow",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxBossFlowTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AMetaPlayerController> PC;
		TWeakObjectPtr<AMetaPlayerCharacter> Player;
		TWeakObjectPtr<AMetaBossBase> Boss;
		UMetaSettingsSubsystem* Settings() const { return World->GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>(); }
	};
	TSharedRef<FState> S = MakeShared<FState>();

	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World())
		{
			S->World = Ctx.World();
		}
	}
	if (!S->World.IsValid() || !S->World->GetMapName().Contains(TEXT("L_Dev_Sandbox")))
	{
		AddError(TEXT("Run this inside L_Dev_Sandbox with -game"));
		return false;
	}

	S->PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(S->World.Get(), 0));
	S->Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(S->World.Get(), 0));
	TActorIterator<AMetaBossBase> BossIt(S->World.Get());
	S->Boss = BossIt ? *BossIt : nullptr;
	if (!S->PC.IsValid() || !S->Player.IsValid() || !S->Boss.IsValid())
	{
		AddError(TEXT("Sandbox needs a MetaPlayerController, MetaPlayerCharacter and one MetaBossBase"));
		return false;
	}

	const UAccessedNoneSettings& Tuning = UAccessedNoneSettings::Get();

	// 1) Pre-set answer, enter the encounter → Armored, setting overridden.
	S->Settings()->SetValue(EMetaSettingKey::Gamma, 1.f);
	S->PC->AN_Encounter();
	TestEqual(TEXT("encounter active"), S->World->GetSubsystem<UEncounterSubsystem>()->IsInCombatEncounter(), true);
	TestEqual(TEXT("boss Armored"), S->Boss->GetGimmick()->GetState(), EGimmickState::Armored);
	TestEqual(TEXT("Gamma forced to 0.2"), S->Settings()->GetValue(EMetaSettingKey::Gamma), 0.2f);
	TestEqual(TEXT("boss HP = D x 10"), S->Boss->GetMaxHealth(), Tuning.KnightAttackPower * Tuning.BossHPMultiplier);
	// Freeze boss AI so it cannot kill the player while we test.
	S->Boss->Patterns->Pause();

	// 2) ESC during combat → x0.15, closing restores x1.
	S->PC->ToggleMenu();
	TestEqual(TEXT("menu dilation in combat"), UGameplayStatics::GetGlobalTimeDilation(S->World.Get()), Tuning.EncounterMenuTimeDilation);
	S->PC->ToggleMenu();
	TestEqual(TEXT("dilation restored"), UGameplayStatics::GetGlobalTimeDilation(S->World.Get()), 1.f);

	// 3) Put the player in front of the boss, facing it.
	const FVector BossLoc = S->Boss->GetActorLocation();
	const FVector Stand = BossLoc + S->Boss->GetActorForwardVector() * 230.f; // outside the 3x boss capsule
	S->Player->SetActorLocation(FVector(Stand.X, Stand.Y, S->Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	S->Player->SetActorRotation((BossLoc - Stand).GetSafeNormal2D().Rotation());

	const float SwingTime = Tuning.LightWindup + Tuning.LightActive + Tuning.LightRecovery + 0.25f;
	auto Swing = [S]() { S->Player->ActivateAbility(UGA_LightAttack::StaticClass()); };

	// 4) Armored: one real light attack (GA + hitbox) = 1% damage.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, 0.2f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		const UAccessedNoneSettings& Tuning = UAccessedNoneSettings::Get();
		const float Expected = Tuning.KnightAttackPower * Tuning.BossHPMultiplier - Tuning.KnightAttackPower * (1.f - Tuning.ArmoredDamageReduction);
		TestNearlyEqual(TEXT("armored boss takes 1%"), S->Boss->GetHealth(), Expected, 0.01f);
		S->Settings()->SetValue(EMetaSettingKey::Gamma, 0.95f);
		TestEqual(TEXT("Gamma 0.95 → Breaking"), S->Boss->GetGimmick()->GetState(), EGimmickState::Breaking);
		// Full HP again so "10 hits" is measured from max.
		S->Boss->GetAbilitySystemComponent()->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), S->Boss->GetMaxHealth());
	}, SwingTime));

	// 5) After BreakingDuration → Vulnerable; settings revert does not matter.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestEqual(TEXT("Vulnerable after breaking"), S->Boss->GetGimmick()->GetState(), EGimmickState::Vulnerable);
		S->Settings()->SetValue(EMetaSettingKey::Gamma, 0.5f);
		S->Boss->Patterns->Pause(); // Vulnerable resumes patterns; keep the boss passive.
	}, S->Boss->GetGimmick()->BreakingDuration + 0.5f));

	// 6) Nine light attacks leave it alive, the tenth kills it.
	for (int32 i = 0; i < 9; ++i)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, i == 0 ? 0.1f : SwingTime));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestTrue(TEXT("alive after 9 hits"), S->Boss->GetHealth() > 0.f);
		TestEqual(TEXT("still latched Vulnerable"), S->Boss->GetGimmick()->GetState(), EGimmickState::Vulnerable);
	}, SwingTime));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand(Swing, 0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestEqual(TEXT("boss Defeated on 10th hit"), S->Boss->GetGimmick()->GetState(), EGimmickState::Defeated);
		TestFalse(TEXT("encounter over"), S->World->GetSubsystem<UEncounterSubsystem>()->IsInCombatEncounter());
	}, SwingTime));
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

/**
 * User revision: the sandbox boss is a practice dummy (no attacks, dies with a motion, vanishes 2 s later, revives),
 * hero reach (1 = 4 < 3) and hero 2 shooting arrows at the aim point.
 *   UnrealEditor AccessedNone.uproject /Game/Maps/L_Dev_Sandbox -game -windowed -resx=1280 -resy=720
 *     -ExecCmds="Automation RunTests AccessedNone.Sandbox.Practice; Quit"
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxPracticeTest, "AccessedNone.Sandbox.Practice",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxPracticeTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AMetaPlayerController> PC;
		TWeakObjectPtr<AMetaPlayerCharacter> Player;
		TWeakObjectPtr<AMetaBossBase> Boss;
		FVector BossSpawn;
		float PlayerHP = 0.f;
		float BossHP = 0.f;
	};
	// Paragon asset-pack Blueprints fail to compile on 5.8 (pre-existing, see DECISIONS.md).
	AddExpectedError(TEXT("ResetOrientationAndPosition"), EAutomationExpectedErrorFlags::Contains, 0);
	TSharedRef<FState> S = MakeShared<FState>();
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { S->World = Ctx.World(); }
	}
	if (!S->World.IsValid() || !S->World->GetMapName().Contains(TEXT("L_Dev_Sandbox")))
	{
		AddError(TEXT("Run this inside L_Dev_Sandbox with -game"));
		return false;
	}
	S->PC = Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(S->World.Get(), 0));
	S->Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(S->World.Get(), 0));
	TActorIterator<AMetaBossBase> BossIt(S->World.Get());
	S->Boss = BossIt ? *BossIt : nullptr;
	if (!S->PC.IsValid() || !S->Player.IsValid() || !S->Boss.IsValid())
	{
		AddError(TEXT("Sandbox needs the player and the dummy"));
		return false;
	}

	// Reach: heroes 1 and 4 match, hero 3 reaches further, hero 2 is ranged.
	const TArray<FHeroDef>& Heroes = UAccessedNoneSettings::Get().Heroes;
	TestEqual(TEXT("hero 1 reach == hero 4"), Heroes[0].AttackOffset.X + Heroes[0].AttackExtent.X, Heroes[3].AttackOffset.X + Heroes[3].AttackExtent.X);
	TestTrue(TEXT("hero 3 reaches further"), Heroes[2].AttackOffset.X + Heroes[2].AttackExtent.X > Heroes[0].AttackOffset.X + Heroes[0].AttackExtent.X);
	TestTrue(TEXT("hero 2 ranged"), Heroes[1].bRanged);

	// Dummy: stands still and never hurts the player.
	S->BossSpawn = S->Boss->GetActorLocation();
	const FVector Stand = S->BossSpawn + S->Boss->GetActorForwardVector() * 230.f;
	S->Player->SetActorLocation(FVector(Stand.X, Stand.Y, S->Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	S->PlayerHP = S->Player->GetHealth();
	S->PC->AN_Encounter();
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestEqual(TEXT("dummy did not attack"), S->Player->GetHealth(), S->PlayerHP);
		TestTrue(TEXT("dummy did not move"), FVector::Dist2D(S->Boss->GetActorLocation(), S->BossSpawn) < 10.f);
		TestEqual(TEXT("dummy in combat state"), S->Boss->GetGimmick()->GetState(), EGimmickState::Armored);

		// Hero 2: back off, lock on, shoot.
		S->Player->SetHero(1);
		const FVector Back = S->BossSpawn + S->Boss->GetActorForwardVector() * 1000.f;
		S->Player->SetActorLocation(FVector(Back.X, Back.Y, S->Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		S->Player->SetActorRotation((S->BossSpawn - Back).GetSafeNormal2D().Rotation());
		S->PC->SetControlRotation((S->BossSpawn - Back).GetSafeNormal2D().Rotation());
	}, 3.f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([S]()
	{
		FScreenshotRequest::RequestScreenshot(TEXT("Practice_Aim"), true, false);
		S->BossHP = S->Boss->GetHealth();
		// Body faces sideways (as if strafing); the shot must turn it toward the dummy.
		S->Player->SetActorRotation(S->Player->GetActorRotation() + FRotator(0.f, 90.f, 0.f));
		S->Player->ActivateAbility(UGA_LightAttack::StaticClass());
	}, 1.5f));
	{
		// Poll: the turn must happen while the attack is still running (real frames can hitch on first FX loads).
		TSharedRef<double> Start = MakeShared<double>(-1.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, S, Start]()
		{
			if (*Start < 0.0) { *Start = FPlatformTime::Seconds(); }
			const FVector To = (S->BossSpawn - S->Player->GetActorLocation()).GetSafeNormal2D();
			const bool bFacing = FVector::DotProduct(S->Player->GetActorForwardVector().GetSafeNormal2D(), To) > 0.95f;
			const bool bAttacking = !S->Player->GetCharacterMovement()->bOrientRotationToMovement;
			if (bFacing && bAttacking) { return true; }
			if (!bAttacking || FPlatformTime::Seconds() - *Start > 3.0)
			{
				AddError(TEXT("archer did not face the target during the shot"));
				return true;
			}
			return false;
		}));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		FScreenshotRequest::RequestScreenshot(TEXT("Practice_Arrow"), true, false);
		TestTrue(TEXT("move-orientation restored after the shot"), S->Player->GetCharacterMovement()->bOrientRotationToMovement);
		TestTrue(TEXT("arrow hit the dummy from 10 m"), S->Boss->GetHealth() < S->BossHP);
		// Kill it: death motion, then vanish, then revive.
		UAbilitySystemComponent* Src = S->Player->GetAbilitySystemComponent();
		const FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, Src->MakeEffectContext());
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, S->Boss->GetAbilitySystemComponent());
		TestTrue(TEXT("dummy dead"), S->Boss->IsDead());
	}, 1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		FScreenshotRequest::RequestScreenshot(TEXT("Practice_Death"), true, false);
		TestFalse(TEXT("body still there right after dying"), S->Boss->IsHidden());
	}, 0.8f));
	// Vanish (motion + 2 s) and revive (+1.5 s) depend on the death clip length: poll with timeouts.
	auto Poll = [this](const FString& What, double Timeout, TFunction<bool()> Cond)
	{
		TSharedRef<double> Start = MakeShared<double>(-1.0);
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, What, Timeout, Cond, Start]()
		{
			if (*Start < 0.0) { *Start = FPlatformTime::Seconds(); }
			if (Cond()) { return true; }
			if (FPlatformTime::Seconds() - *Start > Timeout) { AddError(TEXT("Timed out: ") + What); return true; }
			return false;
		}));
	};
	Poll(TEXT("dummy vanishes"), 10.0, [S]() { return S->Boss->IsHidden(); });
	Poll(TEXT("dummy revives"), 5.0, [S]() { return !S->Boss->IsHidden(); });
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		TestFalse(TEXT("revived alive"), S->Boss->IsDead());
		TestEqual(TEXT("revived into combat"), S->Boss->GetGimmick()->GetState(), EGimmickState::Armored);
		FScreenshotRequest::RequestScreenshot(TEXT("Practice_Revived"), true, false);
	}, 0.5f));
	// Light vs. full-charge heavy motion per hero (visual check, Saved/Screenshots).
	for (int32 Hero = 0; Hero < 4; ++Hero)
	{
		for (const bool bHeavy : { false, true })
		{
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([S, Hero, bHeavy]()
			{
				// Melee range, facing the dummy, camera from the side-back so the contact spark is visible.
				const FVector Stand = S->BossSpawn + S->Boss->GetActorForwardVector() * 240.f;
				S->Player->SetActorLocation(FVector(Stand.X, Stand.Y, S->Player->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
				S->Player->SetActorRotation((S->BossSpawn - Stand).GetSafeNormal2D().Rotation());
				S->PC->SetControlRotation((S->BossSpawn - Stand).GetSafeNormal2D().Rotation() + FRotator(-10.f, 35.f, 0.f));
				S->Player->SetHero(Hero);
				S->Player->ActivateAbility(bHeavy ? UGA_HeavyAttack::StaticClass() : UGA_LightAttack::StaticClass());
			}, 1.2f));
			ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Hero, bHeavy]()
			{
				FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("Motion_Hero%d_%s"), Hero + 1, bHeavy ? TEXT("Heavy") : TEXT("Light")), true, false);
			}, bHeavy ? UAccessedNoneSettings::Get().HeavyWindup + 0.08f : UAccessedNoneSettings::Get().LightWindup + 0.08f)); // just after impact
		}
	}
	// Enemy hit on the player → flinch.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([S]()
	{
		S->Player->SetHero(0);
		UAbilitySystemComponent* Src = S->Boss->GetAbilitySystemComponent();
		FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Src->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, 100.f);
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, S->Player->GetAbilitySystemComponent());
	}, 1.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([]()
	{
		FScreenshotRequest::RequestScreenshot(TEXT("Motion_Flinch"), true, false);
	}, 0.25f));
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Roll rules (user revision): no input = forward roll, stamina cost, blocked at 0 stamina, 1 s cooldown, i-frames;
 * enemy hits make the player flinch.
 *   UnrealEditor-Cmd AccessedNone.uproject /Game/Maps/L_Dev_Sandbox -game -nullrhi -ExecCmds="Automation RunTests AccessedNone.Sandbox.Roll; Quit"
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxRollTest, "AccessedNone.Sandbox.Roll", EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxRollTest::RunTest(const FString& Parameters)
{
	struct FState { TWeakObjectPtr<AMetaPlayerCharacter> Player; TWeakObjectPtr<AMetaBossBase> Boss; FVector Start; FVector Forward; float Stamina = 0.f; };
	TSharedRef<FState> S = MakeShared<FState>();
	UWorld* World = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { World = Ctx.World(); }
	}
	if (!World || !World->GetMapName().Contains(TEXT("L_Dev_Sandbox")))
	{
		AddError(TEXT("Run this inside L_Dev_Sandbox with -game"));
		return false;
	}
	S->Player = Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	TActorIterator<AMetaBossBase> BossIt(World);
	S->Boss = BossIt ? *BossIt : nullptr;
	if (!S->Player.IsValid() || !S->Boss.IsValid())
	{
		AddError(TEXT("Sandbox needs the player and the dummy"));
		return false;
	}
	const UAccessedNoneSettings& T = UAccessedNoneSettings::Get();
	auto Stamina = [S]() { return S->Player->GetAttributes()->GetStamina(); };

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S, Stamina]()
	{
		S->Start = S->Player->GetActorLocation();
		S->Forward = S->Player->GetActorForwardVector().GetSafeNormal2D();
		S->Stamina = Stamina();
		TestTrue(TEXT("roll starts"), S->Player->ActivateAbility(UGA_Dodge::StaticClass()));
		TestTrue(TEXT("i-frames active during the roll"), S->Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(MetaTags::State_IFrame));
		TestNearlyEqual(TEXT("roll costs stamina"), Stamina(), S->Stamina - UAccessedNoneSettings::Get().StaminaCostDodge, 1.f);
	}, 1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		// Still inside the 1 s cooldown.
		S->Player->ActivateAbility(UGA_Dodge::StaticClass());
		TestFalse(TEXT("no back-to-back roll"), S->Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(MetaTags::State_IFrame));
	}, T.DodgeDuration + 0.05f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		const FVector Moved = S->Player->GetActorLocation() - S->Start;
		TestTrue(FString::Printf(TEXT("no input = forward roll (moved %.0f along forward, %.0f total)"), FVector::DotProduct(Moved, S->Forward), Moved.Size2D()),
			FVector::DotProduct(Moved, S->Forward) > 300.f);
		// Out of stamina: blocked even after the cooldown.
		S->Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UMetaCombatAttributeSet::GetStaminaAttribute(), 0.f);
		S->Start = S->Player->GetActorLocation();
		S->Player->ActivateAbility(UGA_Dodge::StaticClass());
		TestFalse(TEXT("no roll at 0 stamina"), S->Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(MetaTags::State_IFrame));
	}, 0.6f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		// An enemy hit lands → flinch.
		S->Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), S->Player->GetMaxHealth());
		UAbilitySystemComponent* Src = S->Boss->GetAbilitySystemComponent();
		FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Src->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, 100.f);
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, S->Player->GetAbilitySystemComponent());
		const UAnimInstance* Anim = S->Player->GetMesh()->GetAnimInstance();
		TestTrue(TEXT("player flinches when hit"), Anim && Anim->IsAnyMontagePlaying());
	}, 0.3f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, S]()
	{
		// Lethal hit → the player collapses (death clip on the mesh) instead of standing.
		UAbilitySystemComponent* Src = S->Boss->GetAbilitySystemComponent();
		const FGameplayEffectSpecHandle Spec = Src->MakeOutgoingSpec(UGE_InstantKill::StaticClass(), 1.f, Src->MakeEffectContext());
		Src->ApplyGameplayEffectSpecToTarget(*Spec.Data, S->Player->GetAbilitySystemComponent());
		TestTrue(TEXT("player dead"), S->Player->IsDead());
		TestEqual(TEXT("player plays the death clip"), S->Player->GetMesh()->GetAnimationMode(), EAnimationMode::AnimationSingleNode);
	}, 0.5f));
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

/** Locking onto the 3x dummy eases the camera back toward CameraArmMax; releasing eases it back in. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxCameraTest, "AccessedNone.Sandbox.Camera", EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxCameraTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { World = Ctx.World(); }
	}
	AMetaPlayerCharacter* P = World ? Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	AMetaPlayerController* PC = World ? Cast<AMetaPlayerController>(UGameplayStatics::GetPlayerController(World, 0)) : nullptr;
	TActorIterator<AMetaBossBase> BossIt(World);
	if (!P || !PC || !BossIt)
	{
		AddError(TEXT("Run this inside L_Dev_Sandbox with -game"));
		return false;
	}
	TWeakObjectPtr<AMetaPlayerCharacter> Player = P;
	const FVector BossLoc = BossIt->GetActorLocation();
	const FVector Stand = BossLoc + BossIt->GetActorForwardVector() * 700.f;
	P->SetActorLocation(FVector(Stand.X, Stand.Y, P->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	PC->SetControlRotation((BossLoc - Stand).GetSafeNormal2D().Rotation());

	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Player]()
	{
		Player->ToggleLockOn();
		TestTrue(TEXT("locked onto the dummy"), Player->IsLockedOn());
	}, 0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Player]()
	{
		const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
		AddInfo(FString::Printf(TEXT("locked arm = %.0f"), Player->GetCameraArmLength()));
		TestTrue(TEXT("camera pulled back for the big boss (>= 650)"), Player->GetCameraArmLength() >= 650.f);
		TestTrue(TEXT("never past the max"), Player->GetCameraArmLength() <= S.CameraArmMax + 1.f);
		Player->ToggleLockOn();
	}, 2.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Player]()
	{
		const UAccessedNoneSettings& S = UAccessedNoneSettings::Get();
		TestNearlyEqual(TEXT("camera back to normal after release"), Player->GetCameraArmLength(), S.CameraArmLength, 15.f);
	}, 2.5f));
	return true;
}

#endif
