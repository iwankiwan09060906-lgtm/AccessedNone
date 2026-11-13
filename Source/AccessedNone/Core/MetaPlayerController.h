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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	class AMetaPlayerCharacter* GetMetaCharacter() const;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnDodge();
	void OnDashStart();
	void OnDashStop();
	void OnJump();
	void OnLockOn();
	void OnLightAttack();
	void OnHeavyAttack();
	void OnUseItem();
};
