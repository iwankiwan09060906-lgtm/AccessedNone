#pragma once

#include "CoreMinimal.h"

/** Which number slot of the Respawn.exe fraction (spec 4.5). */
enum class EMetaNumberSlot : uint8 { Numerator, Denominator };

/**
 * Pure rules of the Phase 5 error window, separate from Slate so tests can drive it:
 * the numerator counts up 1/s; grabbing either number stops the counter and resets the numerator to 1;
 * dropping on the other slot swaps; anything else snaps back and resumes. Solved when value(N)/value(D) > 1.
 */
struct FMetaErrorWindowState
{
	static constexpr int64 InitialDenominator = 99999999;

	int64 Values[2] = { 1, InitialDenominator };
	TOptional<EMetaNumberSlot> Held;
	double CounterAccum = 0.0;
	bool bSolved = false;

	int64 Get(EMetaNumberSlot Slot) const { return Values[static_cast<int32>(Slot)]; }
	double Ratio() const { return Get(EMetaNumberSlot::Denominator) != 0 ? double(Get(EMetaNumberSlot::Numerator)) / double(Get(EMetaNumberSlot::Denominator)) : 0.0; }
	bool IsCounting() const { return !Held.IsSet() && !bSolved; }

	/** Real-time seconds; the numerator grows by one per whole second while counting. */
	void Tick(double Seconds)
	{
		if (!IsCounting())
		{
			return;
		}
		CounterAccum += Seconds;
		while (CounterAccum >= 1.0 && Values[0] < Values[1])
		{
			CounterAccum -= 1.0;
			++Values[0];
		}
	}

	void BeginGrab(EMetaNumberSlot Slot)
	{
		if (bSolved)
		{
			return;
		}
		Held = Slot;
		Values[0] = 1;
		CounterAccum = 0.0;
	}

	/** Target unset (outside / no slot) or the same slot = snap back. Returns true when this drop solved it. */
	bool Drop(TOptional<EMetaNumberSlot> Target)
	{
		if (!Held.IsSet())
		{
			return false;
		}
		const EMetaNumberSlot From = Held.GetValue();
		Held.Reset();
		if (!Target.IsSet() || Target.GetValue() == From)
		{
			return false;
		}
		Swap(Values[0], Values[1]);
		bSolved = Ratio() > 1.0;
		return bSolved;
	}
};
