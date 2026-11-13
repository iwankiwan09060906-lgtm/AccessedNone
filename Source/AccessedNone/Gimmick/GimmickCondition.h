#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Meta/MetaSettingsSubsystem.h"
#include "Meta/MetaConsoleSubsystem.h"
#include "GimmickCondition.generated.h"

class UGimmickComponent;

/** One break condition of a boss gimmick (spec 8.5). All conditions on a component are AND-ed. */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class ACCESSEDNONE_API UGimmickCondition : public UObject
{
	GENERATED_BODY()

public:
	/** Bind delegates. */
	virtual void Activate(UGimmickComponent* InOwner);
	/** Unbind delegates. */
	virtual void Deactivate();
	virtual bool IsSatisfied() const PURE_VIRTUAL(UGimmickCondition::IsSatisfied, return false;);
	/** Called when a try restarts. */
	virtual void ResetCondition() {}

protected:
	void NotifyOwner();
	UGameInstance* GetGameInstance() const;

	TWeakObjectPtr<UGimmickComponent> Owner;
};

UCLASS(meta=(DisplayName="Setting Threshold"))
class ACCESSEDNONE_API UGimmickCondition_SettingThreshold : public UGimmickCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Condition") EMetaSettingKey Key = EMetaSettingKey::Gamma;
	UPROPERTY(EditAnywhere, Category="Condition") float Threshold = 0.95f;
	/** Ignored when bEquals is true. */
	UPROPERTY(EditAnywhere, Category="Condition") bool bGreaterOrEqual = true;
	/** Approximate equality instead of a threshold. */
	UPROPERTY(EditAnywhere, Category="Condition") bool bEquals = false;

	virtual void Activate(UGimmickComponent* InOwner) override;
	virtual void Deactivate() override;
	virtual bool IsSatisfied() const override;

private:
	void HandleSettingChanged(EMetaSettingKey ChangedKey, float OldValue, float NewValue);
	FDelegateHandle Handle;
};

UCLASS(meta=(DisplayName="Console Command"))
class ACCESSEDNONE_API UGimmickCondition_ConsoleCommand : public UGimmickCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Condition") EMetaConsoleCommand RequiredCommand = EMetaConsoleCommand::Slomo;
	UPROPERTY(EditAnywhere, Category="Condition") float TargetValue = 0.05f;
	UPROPERTY(EditAnywhere, Category="Condition") float Tolerance = 0.001f;

	virtual void Activate(UGimmickComponent* InOwner) override;
	virtual void Deactivate() override;
	/** Latched: once the command succeeded in this try it stays satisfied. */
	virtual bool IsSatisfied() const override { return bExecuted; }
	virtual void ResetCondition() override { bExecuted = false; }

private:
	UFUNCTION() void HandleCommand(const FMetaConsoleResult& Result);
	bool bExecuted = false;
};

/** E3. Evaluated when the dialogue ends; the dialogue system (S4-T01) calls EvaluateNames. */
UCLASS(meta=(DisplayName="Dialogue Swap"))
class ACCESSEDNONE_API UGimmickCondition_DialogueSwap : public UGimmickCondition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Condition") FName PlayerSpeakerId = TEXT("Player");
	UPROPERTY(EditAnywhere, Category="Condition") FName TargetSpeakerId = TEXT("Statue");

	virtual bool IsSatisfied() const override { return bSwapped; }
	virtual void ResetCondition() override { bSwapped = false; }

	/** Called at dialogue end with the names currently typed into each speaker tag. */
	void EvaluateNames(const FString& PlayerTagName, const FString& TargetTagName);

	/** Trim, drop all whitespace, case-insensitive alias match (spec E3 input rules). */
	static bool MatchesAlias(const FString& Input, const TArray<FString>& Aliases);

private:
	bool bSwapped = false;
};

/** Phase 5 error window. The widget (S2-T05) calls SetFraction after every drop. */
UCLASS(meta=(DisplayName="UI Drag Drop"))
class ACCESSEDNONE_API UGimmickCondition_UIDragDrop : public UGimmickCondition
{
	GENERATED_BODY()

public:
	virtual bool IsSatisfied() const override { return Denominator > 0.0 && Numerator / Denominator > 1.0; }
	virtual void ResetCondition() override { Numerator = 1.0; Denominator = 1.0; }

	void SetFraction(double InNumerator, double InDenominator);

private:
	double Numerator = 1.0;
	double Denominator = 1.0;
};
