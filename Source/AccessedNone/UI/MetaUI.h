#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

/** Shared look for all C++-built widgets (spec 12.1 colors). Widget trees are built in C++ (MCP can't edit WBP trees). */
namespace MetaUI
{
	inline FLinearColor Hex(const TCHAR* InHex) { return FLinearColor(FColor::FromHex(InHex)); }

	inline const FLinearColor Gold = Hex(TEXT("F2D27A"));
	inline const FLinearColor Crack = Hex(TEXT("8A3FFC"));
	inline const FLinearColor Error = Hex(TEXT("E5243B"));
	inline const FLinearColor Debugger = Hex(TEXT("3DDC97"));
	inline const FLinearColor Panel = FLinearColor(0.02f, 0.02f, 0.03f, 0.85f);

	/** Default Slate font: Roboto + DroidSansFallback (covers Hangul). Typeface "Mono" = DroidSansMono. */
	inline FSlateFontInfo Font(int32 Size, const TCHAR* Typeface = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	}

	inline UTextBlock* Text(UWidgetTree* Tree, const FText& InText, int32 Size, const FLinearColor& Color = FLinearColor::White, const TCHAR* Typeface = TEXT("Regular"))
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>();
		T->SetText(InText);
		T->SetFont(Font(Size, Typeface));
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	}

	/**
	 * True when the C++ tree should be built: empty tree, or only an empty root panel (what a new WBP asset has).
	 * Clears that placeholder root so the caller can set its own.
	 */
	inline bool NeedsBuild(UWidgetTree* Tree)
	{
		if (!Tree)
		{
			return false;
		}
		const UPanelWidget* RootPanel = Cast<UPanelWidget>(Tree->RootWidget);
		if (RootPanel && RootPanel->GetChildrenCount() == 0)
		{
			Tree->RootWidget = nullptr;
		}
		return Tree->RootWidget == nullptr;
	}

	/** Loads /Game/UI/<Name>.<Name>_C (the spec's WBP_ asset, parent = T) or falls back to the C++ class. */
	template <typename T>
	TSubclassOf<T> WidgetClass(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/UI/%s.%s_C"), Name, Name);
		UClass* Loaded = LoadClass<T>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
		return Loaded ? TSubclassOf<T>(Loaded) : TSubclassOf<T>(T::StaticClass());
	}

	/** Seconds since app start, unaffected by time dilation (spec 2.2 implementation note). */
	inline double Now() { return FPlatformTime::Seconds(); }
}
