#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MetaConsoleSubsystem.generated.h"

UENUM(BlueprintType)
enum class EMetaConsoleCommand : uint8 { None, SetGravity, Slomo, Help, Denied, Unknown };

USTRUCT(BlueprintType)
struct FMetaConsoleResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EMetaConsoleCommand Command = EMetaConsoleCommand::None;
	UPROPERTY(BlueprintReadOnly) float Value = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
	/** ST_Game key of the response (CON_*). Lets tests and UI check the outcome without comparing text. */
	UPROPERTY(BlueprintReadOnly) FName TextKey;
	/** Did-you-mean suggestion, if any. */
	UPROPERTY(BlueprintReadOnly) FString Suggestion;
	UPROPERTY(BlueprintReadOnly) FText OutputText;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConsoleCommandExecuted, const FMetaConsoleResult&, Result);

/** In-game virtual developer console (spec 8.3). Not the engine console; whitelist only. */
UCLASS()
class ACCESSEDNONE_API UMetaConsoleSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable) FMetaConsoleResult Execute(const FString& RawInput);
	UFUNCTION(BlueprintCallable) FString Autocomplete(const FString& Partial) const;
	UFUNCTION(BlueprintCallable) void SetUnlocked(bool bInUnlocked) { bUnlocked = bInUnlocked; }
	UFUNCTION(BlueprintPure) bool IsUnlocked() const { return bUnlocked; }

	UPROPERTY(BlueprintAssignable) FOnConsoleCommandExecuted OnCommandExecuted;

	/** Pure parser, no side effects. */
	static FMetaConsoleResult Parse(const FString& RawInput, const FVector2D& SlomoClamp);

private:
	bool bUnlocked = false;
};
