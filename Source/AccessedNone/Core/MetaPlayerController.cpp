#include "Core/MetaPlayerController.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaGameplayAbility.h"
#include "Core/MetaLog.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	constexpr int32 GameplayContextPriority = 0;

	template <typename T>
	T* LoadInputAsset(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("%s%s.%s"), AMetaPlayerController::InputFolder, Name, Name);
		T* Asset = LoadObject<T>(nullptr, *Path);
		UE_CLOG(!Asset, LogAccessedNone, Error, TEXT("Missing input asset %s — run Scripts/Editor/create_input_assets.py"), *Path);
		return Asset;
	}
}

UInputAction* AMetaPlayerController::LoadAction(const TCHAR* Name) { return LoadInputAsset<UInputAction>(Name); }
UInputMappingContext* AMetaPlayerController::LoadContext(const TCHAR* Name) { return LoadInputAsset<UInputMappingContext>(Name); }

AMetaPlayerCharacter* AMetaPlayerController::GetMetaCharacter() const
{
	return Cast<AMetaPlayerCharacter>(GetPawn());
}

void AMetaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (UInputMappingContext* Gameplay = LoadContext(TEXT("IMC_Gameplay")))
		{
			Input->AddMappingContext(Gameplay, GameplayContextPriority);
		}
	}
}

void AMetaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}

	auto Bind = [EIC, this](const TCHAR* Name, ETriggerEvent Event, void (AMetaPlayerController::*Func)())
	{
		if (UInputAction* IA = LoadAction(Name)) { EIC->BindAction(IA, Event, this, Func); }
	};

	if (UInputAction* IA = LoadAction(TEXT("IA_Move"))) { EIC->BindAction(IA, ETriggerEvent::Triggered, this, &AMetaPlayerController::OnMove); }
	if (UInputAction* IA = LoadAction(TEXT("IA_Look"))) { EIC->BindAction(IA, ETriggerEvent::Triggered, this, &AMetaPlayerController::OnLook); }
	Bind(TEXT("IA_Dodge"), ETriggerEvent::Triggered, &AMetaPlayerController::OnDodge);
	Bind(TEXT("IA_Dash"), ETriggerEvent::Triggered, &AMetaPlayerController::OnDashStart);
	Bind(TEXT("IA_Dash"), ETriggerEvent::Completed, &AMetaPlayerController::OnDashStop);
	Bind(TEXT("IA_Dash"), ETriggerEvent::Canceled, &AMetaPlayerController::OnDashStop);
	Bind(TEXT("IA_Jump"), ETriggerEvent::Triggered, &AMetaPlayerController::OnJump);
	Bind(TEXT("IA_LockOn"), ETriggerEvent::Triggered, &AMetaPlayerController::OnLockOn);
	Bind(TEXT("IA_LightAttack"), ETriggerEvent::Triggered, &AMetaPlayerController::OnLightAttack);
	Bind(TEXT("IA_HeavyAttack"), ETriggerEvent::Triggered, &AMetaPlayerController::OnHeavyAttack);
	Bind(TEXT("IA_UseItem"), ETriggerEvent::Triggered, &AMetaPlayerController::OnUseItem);
}

void AMetaPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->Move(Value.Get<FVector2D>()); }
}

void AMetaPlayerController::OnLook(const FInputActionValue& Value)
{
	if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->Look(Value.Get<FVector2D>()); }
}

void AMetaPlayerController::OnDodge()       { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->ActivateAbility(UGA_Dodge::StaticClass()); } }
void AMetaPlayerController::OnDashStart()   { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->SetDashing(true); } }
void AMetaPlayerController::OnDashStop()    { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->SetDashing(false); } }
void AMetaPlayerController::OnJump()        { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->TryJump(); } }
void AMetaPlayerController::OnLockOn()      { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->ToggleLockOn(); } }
void AMetaPlayerController::OnLightAttack() { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->ActivateAbility(UGA_LightAttack::StaticClass()); } }
void AMetaPlayerController::OnHeavyAttack() { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->ActivateAbility(UGA_HeavyAttack::StaticClass()); } }
void AMetaPlayerController::OnUseItem()     { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->ActivateAbility(UGA_Heal::StaticClass()); } }
