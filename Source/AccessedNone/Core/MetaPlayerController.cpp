#include "Core/MetaPlayerController.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Characters/MetaBossBase.h"
#include "Characters/BossDataAsset.h"
#include "Combat/MetaGameplayAbility.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaLog.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Gimmick/EncounterTriggerBox.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/MetaText.h"
#include "UI/MetaDebuggerWidget.h"
#include "UI/MetaHUDWidget.h"
#include "UI/MetaSettingsWidget.h"
#include "UI/MetaUI.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	constexpr int32 GameplayContextPriority = 0;
	constexpr int32 MenuContextPriority = 10;
	constexpr int32 UIManipulationContextPriority = 20;
	constexpr int32 HUDZOrder = 0;
	constexpr int32 DebuggerZOrder = 5;

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
	SetGameplayInputEnabled(true);
	if (!IsLocalController())
	{
		return;
	}
	if (!MenuWidgetClass)
	{
		MenuWidgetClass = MetaUI::WidgetClass<UMetaSettingsWidget>(TEXT("WBP_Settings"));
	}
	HUDWidget = CreateWidget<UMetaHUDWidget>(this, MetaUI::WidgetClass<UMetaHUDWidget>(TEXT("WBP_HUD")));
	HUDWidget->AddToViewport(HUDZOrder);
	DebuggerWidget = CreateWidget<UMetaDebuggerWidget>(this, MetaUI::WidgetClass<UMetaDebuggerWidget>(TEXT("WBP_Debugger")));
	DebuggerWidget->AddToViewport(DebuggerZOrder);
}

void AMetaPlayerController::SetUIManipulation(bool bEnabled)
{
	UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	UInputMappingContext* Gameplay = LoadContext(TEXT("IMC_Gameplay"));
	UInputMappingContext* UI = LoadContext(TEXT("IMC_UIManipulation"));
	bUIManipulation = bEnabled;
	if (Input && Gameplay && UI)
	{
		if (bEnabled)
		{
			Input->RemoveMappingContext(Gameplay);
			Input->AddMappingContext(UI, UIManipulationContextPriority);
		}
		else
		{
			Input->RemoveMappingContext(UI);
			Input->AddMappingContext(Gameplay, GameplayContextPriority);
		}
	}
	if (bEnabled)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	SetShowMouseCursor(bEnabled);
}

void AMetaPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (IsMenuOpen())
	{
		ToggleMenu(); // never leave the world slowed
	}
	Super::EndPlay(Reason);
}

void AMetaPlayerController::SetGameplayInputEnabled(bool bEnabled)
{
	UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Input)
	{
		return;
	}
	UInputMappingContext* Gameplay = LoadContext(TEXT("IMC_Gameplay"));
	UInputMappingContext* Menu = LoadContext(TEXT("IMC_Menu"));
	if (bEnabled)
	{
		if (Menu) { Input->RemoveMappingContext(Menu); }
		if (Gameplay) { Input->AddMappingContext(Gameplay, GameplayContextPriority); }
	}
	else
	{
		if (Gameplay) { Input->RemoveMappingContext(Gameplay); }
		if (Menu) { Input->AddMappingContext(Menu, MenuContextPriority); }
	}
}

void AMetaPlayerController::ToggleMenu()
{
	if (IsMenuOpen())
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		SetGameplayInputEnabled(true);
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
		return;
	}

	if (GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->IsMenuLocked())
	{
		UE_LOG(LogAccessedNone, Log, TEXT("Menu locked: Access Denied"));
		if (HUDWidget) { HUDWidget->ShowToast(MetaText::Get(TEXT("UI_ACCESS_DENIED"))); }
		return;
	}
	if (bUIManipulation)
	{
		return;
	}

	const UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>();
	const bool bInCombat = Encounters && Encounters->IsInCombatEncounter();
	UGameplayStatics::SetGlobalTimeDilation(this, bInCombat ? UAccessedNoneSettings::Get().EncounterMenuTimeDilation : 1.f);

	if (MenuWidgetClass)
	{
		MenuWidget = CreateWidget<UUserWidget>(this, MenuWidgetClass);
		MenuWidget->AddToViewport(MenuContextPriority);
	}
	SetGameplayInputEnabled(false);
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
	UE_LOG(LogAccessedNone, Log, TEXT("Menu opened (combat=%d, time dilation=%.2f)"), bInCombat, UGameplayStatics::GetGlobalTimeDilation(this));
}

void AMetaPlayerController::AN_Set(const FString& Key, float Value)
{
	const int64 EnumValue = StaticEnum<EMetaSettingKey>()->GetValueByNameString(Key);
	if (EnumValue == INDEX_NONE)
	{
		UE_LOG(LogAccessedNone, Warning, TEXT("AN_Set: unknown key '%s'"), *Key);
		return;
	}
	GetGameInstance()->GetSubsystem<UMetaSettingsSubsystem>()->SetValue(static_cast<EMetaSettingKey>(EnumValue), Value);
}

void AMetaPlayerController::AN_Encounter()
{
	for (TActorIterator<AEncounterTriggerBox> It(GetWorld()); It; ++It)
	{
		It->StartEncounter();
	}
}

void AMetaPlayerController::AN_Status()
{
	const UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>();
	UE_LOG(LogAccessedNone, Display, TEXT("[AN_Status] dilation=%.2f menu=%d encounter=%s"),
		UGameplayStatics::GetGlobalTimeDilation(this), IsMenuOpen(), *Encounters->GetActiveEncounterId().ToString());
	if (const AMetaPlayerCharacter* C = GetMetaCharacter())
	{
		UE_LOG(LogAccessedNone, Display, TEXT("[AN_Status] player HP=%.0f/%.0f dead=%d"), C->GetHealth(), C->GetMaxHealth(), C->IsDead());
	}
	for (TActorIterator<AMetaBossBase> It(GetWorld()); It; ++It)
	{
		FGameplayTagContainer OwnedTags;
		It->GetAbilitySystemComponent()->GetOwnedGameplayTags(OwnedTags);
		UE_LOG(LogAccessedNone, Display, TEXT("[AN_Status] %s state=%s HP=%.1f/%.0f DR=%.2f tags=%s effects=%d data=%s"), *It->GetName(),
			*UEnum::GetValueAsString(It->GetGimmick()->GetState()), It->GetHealth(), It->GetMaxHealth(),
			It->GetAttributes()->GetDamageReduction(), *OwnedTags.ToStringSimple(), It->GetAbilitySystemComponent()->GetActiveEffects(FGameplayEffectQuery()).Num(), *GetNameSafe(It->GetBossData()));
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
	Bind(TEXT("IA_Menu"), ETriggerEvent::Triggered, &AMetaPlayerController::OnMenu);

	// Dev hero switch (keys 1-4): raw key bindings, no input asset needed.
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AMetaPlayerController::OnSelectHero<0>);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AMetaPlayerController::OnSelectHero<1>);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AMetaPlayerController::OnSelectHero<2>);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AMetaPlayerController::OnSelectHero<3>);
}

template <int32 Index>
void AMetaPlayerController::OnSelectHero() { if (AMetaPlayerCharacter* C = GetMetaCharacter()) { C->SetHero(Index); } }

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
