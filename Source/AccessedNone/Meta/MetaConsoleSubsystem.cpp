#include "Meta/MetaConsoleSubsystem.h"
#include "Core/AccessedNoneSettings.h"
#include "Core/MetaLog.h"
#include "Core/MetaText.h"
#include "Algo/LevenshteinDistance.h"

namespace
{
	const TCHAR* const AutocompleteTargets[] = { TEXT("set gravity "), TEXT("slomo "), TEXT("help") };
	constexpr int32 MaxSuggestDistance = 2;

	bool TryParseFiniteFloat(const FString& Token, float& Out)
	{
		return LexTryParseString(Out, *Token) && FMath::IsFinite(Out);
	}

	FMetaConsoleResult Make(EMetaConsoleCommand Command, bool bSuccess, const TCHAR* Key, float Value = 0.f)
	{
		FMetaConsoleResult R;
		R.Command = Command;
		R.bSuccess = bSuccess;
		R.Value = Value;
		R.TextKey = Key;
		return R;
	}

	/** Closest whitelisted command within MaxSuggestDistance, or empty. */
	FString FindSuggestion(const TArray<FString>& Tokens)
	{
		const FString First = Tokens[0];
		const FString FirstTwo = Tokens.Num() >= 2 ? Tokens[0] + TEXT(" ") + Tokens[1] : First;

		struct FCandidate { const TCHAR* Name; const FString* Compare; };
		const FCandidate Candidates[] = {
			{ TEXT("set gravity"), &FirstTwo },
			{ TEXT("slomo"), &First },
			{ TEXT("help"), &First } };

		FString Best;
		int32 BestDist = MaxSuggestDistance + 1;
		for (const FCandidate& C : Candidates)
		{
			const int32 Dist = Algo::LevenshteinDistance(*C.Compare, FString(C.Name));
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Best = C.Name;
			}
		}
		return Best;
	}
}

FMetaConsoleResult UMetaConsoleSubsystem::Parse(const FString& RawInput, const FVector2D& SlomoClamp)
{
	const FString Input = RawInput.TrimStartAndEnd().ToLower();
	TArray<FString> Tokens;
	Input.ParseIntoArrayWS(Tokens);

	FMetaConsoleResult R;
	if (Tokens.IsEmpty())
	{
		return R;
	}

	float Number = 0.f;
	const FString& Cmd = Tokens[0];

	if (Cmd == TEXT("set") && Tokens.Num() >= 2 && Tokens[1] == TEXT("gravity"))
	{
		if (Tokens.Num() != 3 || !TryParseFiniteFloat(Tokens[2], Number))
		{
			R = Make(EMetaConsoleCommand::SetGravity, false, TEXT("CON_USAGE"));
		}
		else if (Number <= 0.f)
		{
			R = Make(EMetaConsoleCommand::SetGravity, false, TEXT("CON_GRAVITY_REJECT"), Number);
		}
		else
		{
			R = Make(EMetaConsoleCommand::SetGravity, true, TEXT("CON_GRAVITY_OK"), Number);
		}
	}
	else if (Cmd == TEXT("slomo"))
	{
		if (Tokens.Num() != 2 || !TryParseFiniteFloat(Tokens[1], Number))
		{
			R = Make(EMetaConsoleCommand::Slomo, false, TEXT("CON_USAGE"));
		}
		else
		{
			R = Make(EMetaConsoleCommand::Slomo, true, TEXT("CON_SLOMO_OK"), FMath::Clamp<float>(Number, SlomoClamp.X, SlomoClamp.Y));
		}
	}
	else if (Cmd == TEXT("help") && Tokens.Num() == 1)
	{
		R = Make(EMetaConsoleCommand::Help, true, TEXT("CON_HELP"));
	}
	else if (Cmd == TEXT("god") || Cmd == TEXT("kill"))
	{
		R = Make(EMetaConsoleCommand::Denied, false, TEXT("CON_DENIED"));
	}
	else
	{
		const FString Suggestion = FindSuggestion(Tokens);
		R = Make(EMetaConsoleCommand::Unknown, false, Suggestion.IsEmpty() ? TEXT("CON_UNKNOWN") : TEXT("CON_DID_YOU_MEAN"));
		R.Suggestion = Suggestion;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Input"), FText::FromString(RawInput.TrimStartAndEnd()));
	Args.Add(TEXT("Suggestion"), FText::FromString(R.Suggestion));
	Args.Add(TEXT("Value"), FText::AsNumber(R.Value));
	R.OutputText = FText::Format(MetaText::Get(*R.TextKey.ToString()), Args);
	return R;
}

FMetaConsoleResult UMetaConsoleSubsystem::Execute(const FString& RawInput)
{
	const FMetaConsoleResult Result = Parse(RawInput, UAccessedNoneSettings::Get().SlomoClamp);
	UE_LOG(LogMetaConsole, Log, TEXT("> %s => %s (%s)"), *RawInput, *Result.TextKey.ToString(), Result.bSuccess ? TEXT("ok") : TEXT("fail"));
	if (Result.Command != EMetaConsoleCommand::None)
	{
		OnCommandExecuted.Broadcast(Result);
	}
	return Result;
}

FString UMetaConsoleSubsystem::Autocomplete(const FString& Partial) const
{
	const FString Lower = Partial.TrimStart().ToLower();
	if (!Lower.IsEmpty())
	{
		for (const TCHAR* Target : AutocompleteTargets)
		{
			if (FString(Target).StartsWith(Lower))
			{
				return Target;
			}
		}
	}
	return Partial;
}
