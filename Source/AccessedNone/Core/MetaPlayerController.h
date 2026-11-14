#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MetaPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Owns all input bindings (spec ch.10) and forwards gameplay input to the possessed AMetaPlayerCharacter,
 * so possessing AStatuePawn (E3) keeps every control working.
 */
UCLASS()
class ACCESSEDNONE_API AMetaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Folder holding IA_* / IMC_* assets (created by Scripts/Editor/create_input_assets.py). */
	static constexpr const TCHAR* InputFolder = TEXT("/Game/Core/Input/");

	static UInputAction* LoadAction(const TCHAR* Name);
	static UInputMappingContext* LoadContext(const TCHAR* Name);

	/** ESC. During a combat encounter the world slows to EncounterMenuTimeDilation (spec 2.2). */
	UFUNCTION(BlueprintCallable, Category="Menu") void ToggleMenu();
	UFUNCTION(BlueprintPure, Category="Menu") bool IsMenuOpen() const { return MenuWidget != nullptr; }

	// --- Dev commands (spec S1 DoD allows temporary debug commands) ---
	/** an.Set Gamma 0.95 — sets a virtual setting as if the player used the menu. */
	UFUNCTION(Exec) void AN_Set(const FString& Key, float Value);
	UFUNCTION(Exec) void AN_Menu() { ToggleMenu(); }
	/** Logs time dilation, encounter and player/boss state. */
	UFUNCTION(Exec) void AN_Status();
	/** Starts every encounter trigger in the level as if the player walked in. */
	UFUNCTION(Exec) void AN_Encounter();

	UPROPERTY(EditDefaultsOnly, Category="Menu") TSubclassOf<class UUserWidget> MenuWidgetClass;

	UFUNCTION(BlueprintPure, Category="UI") class UMetaHUDWidget* GetHUDWidget() const { return HUDWidget; }
	/** Error window / name tag editing: IMC_UIManipulation replaces gameplay input, cursor shown (spec 8.8). */
	UFUNCTION(BlueprintCallable, Category="UI") void SetUIManipulation(bool bEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	class AMetaPlayerCharacter* GetMetaCharacter() const;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnDodge();
	void OnJump();
	void OnLockOn();
	void OnLightAttack();
	void OnHeavyAttack();
	void OnUseItem();
	void OnMenu() { ToggleMenu(); }
	template <int32 Index> void OnSelectHero();

	void SetGameplayInputEnabled(bool bEnabled);

	UPROPERTY() TObjectPtr<class UUserWidget> MenuWidget;
	UPROPERTY() TObjectPtr<class UMetaHUDWidget> HUDWidget;
	UPROPERTY() TObjectPtr<class UMetaDebuggerWidget> DebuggerWidget;
	bool bUIManipulation = false;
};
