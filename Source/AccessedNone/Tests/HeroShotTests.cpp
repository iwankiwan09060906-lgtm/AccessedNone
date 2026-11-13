#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/SkeletalMeshActor.h"
#include "Camera/CameraActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Core/AccessedNoneSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Visual check of every hero (needs rendering, so no -nullrhi):
 *   UnrealEditor-Cmd AccessedNone.uproject /Game/Maps/L_Dev_Sandbox -game -windowed -ResX=1280 -ResY=720
 *     -ExecCmds="Automation RunTests AccessedNone.Sandbox.HeroShots; Quit"
 * Left = player after SetHero, right = same mesh playing its raw Idle sequence (no anim BP). Screenshots: Saved/Screenshots.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxHeroShotsTest, "AccessedNone.Sandbox.HeroShots",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxHeroShotsTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { World = Ctx.World(); }
	}
	AMetaPlayerCharacter* Player = World ? Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	if (!Player)
	{
		AddError(TEXT("Run inside L_Dev_Sandbox with -game"));
		return false;
	}

	const FVector P = Player->GetActorLocation();
	ACameraActor* Cam = World->SpawnActor<ACameraActor>(P + FVector(450.f, 90.f, 40.f), (FVector(-450.f, 0.f, -40.f)).Rotation());
	UGameplayStatics::GetPlayerController(World, 0)->SetViewTarget(Cam);
	Player->SetActorRotation(FRotator(0.f, 0.f, 0.f));

	TSharedRef<TWeakObjectPtr<ASkeletalMeshActor>> Ref = MakeShared<TWeakObjectPtr<ASkeletalMeshActor>>();
	const int32 Num = UAccessedNoneSettings::Get().Heroes.Num();
	for (int32 i = 0; i < Num; ++i)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([=]()
		{
			const FHeroDef& H = UAccessedNoneSettings::Get().Heroes[i];
			Player->SetHero(i);
			if (Ref->IsValid()) { (*Ref)->Destroy(); }
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASkeletalMeshActor* A = World->SpawnActor<ASkeletalMeshActor>(P + FVector(0.f, 180.f, -97.f), FRotator(0.f, -90.f, 0.f), Params);
			USkeletalMeshComponent* C = A->GetSkeletalMeshComponent();
			C->SetSkeletalMeshAsset(H.Mesh.LoadSynchronous());
			// Reference: the hero's raw Idle sequence (no anim BP), to see what the anim data itself does.
			const FString Idle = FString::Printf(TEXT("/Game/Paragon%s/Characters/Heroes/%s/Animations/Idle.Idle"), *H.Id.ToString(), *H.Id.ToString());
			C->PlayAnimation(LoadObject<UAnimationAsset>(nullptr, *Idle), true);
			*Ref = A;
			return true;
		}));
		// Time series after the swap: intro montages / cloth settle within a few seconds.
		for (const float T : { 0.3f, 1.5f, 4.f })
		{
			ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(T < 1.f ? T : T - (T < 2.f ? 0.3f : 1.5f)));
			ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([i, T, Player, Ref]()
			{
				const USkeletalMeshComponent* PM = Player->GetMesh();
				const UAnimInstance* PA = PM->GetAnimInstance();
				const USkeletalMeshComponent* RM = Ref->IsValid() ? (*Ref)->GetSkeletalMeshComponent() : nullptr;
				UE_LOG(LogTemp, Display, TEXT("[HeroShot] %d %.1fs player mesh=%s anim=%s montage=%s overrides=%d | ref mesh=%s anim=%s montage=%s"), i + 1, T,
					*GetNameSafe(PM->GetSkeletalMeshAsset()), *GetNameSafe(PA), PA ? *GetNameSafe(PA->GetCurrentActiveMontage()) : TEXT("-"), PM->OverrideMaterials.Num(),
					RM ? *GetNameSafe(RM->GetSkeletalMeshAsset()) : TEXT("-"), RM ? *GetNameSafe(RM->GetAnimInstance()) : TEXT("-"),
					RM && RM->GetAnimInstance() ? *GetNameSafe(RM->GetAnimInstance()->GetCurrentActiveMontage()) : TEXT("-"));
				FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("Hero_%d_%ds"), i + 1, FMath::RoundToInt(T * 10.f)), false, false);
				return true;
			}));
		}
	}
	return true;
}

/**
 * Terra (key 4) in motion, seen through the player's own follow camera: idle → run → dash → jump → idle.
 * Same command line as HeroShots with AccessedNone.Sandbox.HeroMotion. Screenshots Motion_<step>.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSandboxHeroMotionTest, "AccessedNone.Sandbox.HeroMotion",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FSandboxHeroMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = nullptr;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::Game && Ctx.World()) { World = Ctx.World(); }
	}
	AMetaPlayerCharacter* Player = World ? Cast<AMetaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	if (!Player)
	{
		AddError(TEXT("Run inside L_Dev_Sandbox with -game"));
		return false;
	}
	const int32 Hero = Parameters.IsEmpty() ? 3 : FCString::Atoi(*Parameters);
	Player->GetController()->SetControlRotation(FRotator(-10.f, 90.f, 0.f)); // run sideways so the camera sees the profile

	auto Shot = [](const TCHAR* Name)
	{
		return new FFunctionLatentCommand([Name = FString(Name)]() { FScreenshotRequest::RequestScreenshot(TEXT("Motion_") + Name, false, false); return true; });
	};
	auto Run = [Player](float Seconds, bool bDash)
	{
		TSharedRef<double> End = MakeShared<double>(-1.0);
		return new FFunctionLatentCommand([Player, Seconds, bDash, End]()
		{
			const double Now = FPlatformTime::Seconds();
			if (*End < 0.0) { *End = Now + Seconds; Player->SetDashing(bDash); }
			Player->Move(FVector2D(0.f, 1.f));
			if (Now < *End) { return false; }
			Player->SetDashing(false);
			return true;
		});
	};

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Player, Hero, World]()
	{
		Player->SetHero(Hero);
		// Reference: Paragon's own character BP for this hero, standing beside the player.
		const FString Id = UAccessedNoneSettings::Get().Heroes[Hero].Id.ToString();
		if (UClass* BP = LoadClass<AActor>(nullptr, *FString::Printf(TEXT("/Game/Paragon%s/Characters/Heroes/%s/%sPlayerCharacter.%sPlayerCharacter_C"), *Id, *Id, *Id, *Id)))
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<AActor>(BP, Player->GetActorLocation() + FVector(-250.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), Params);
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Shot(TEXT("1_Idle"))));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Player]() { USkeletalMeshComponent* M = Player->GetMesh(); M->bDisableClothSimulation = false; M->RecreateClothingActors(); return true; }));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Run(1.0f, false)));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Shot(TEXT("2_Run"))));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Run(1.0f, true)));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Shot(TEXT("3_Dash"))));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Player]() { Player->TryJump(); return true; }));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Shot(TEXT("4_Jump"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShareable(Shot(TEXT("5_IdleAfter"))));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	return true;
}

#endif
