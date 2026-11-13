#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags CombatTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** Throwaway game world with bare actors carrying an ASC + attribute set. */
	struct FCombatFixture
	{
		UWorld* World = nullptr;

		FCombatFixture()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}

		~FCombatFixture()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		UAbilitySystemComponent* SpawnKnight()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor);
			ASC->RegisterComponent();
			ASC->InitAbilityActorInfo(Actor, Actor);
			ASC->AddSpawnedAttribute(NewObject<UMetaCombatAttributeSet>(Actor));
			Apply(ASC, UGE_Init_Knight::StaticClass());
			return ASC;
		}

		/** Boss HP = attacker D × multiplier (spec 3.1). */
		UAbilitySystemComponent* SpawnBoss(float AttackerPower, float Multiplier, std::initializer_list<TSubclassOf<UGameplayEffect>> Effects)
		{
			UAbilitySystemComponent* ASC = SpawnKnight();
			ASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetMaxHealthAttribute(), AttackerPower * Multiplier);
			ASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), AttackerPower * Multiplier);
			for (const TSubclassOf<UGameplayEffect>& GE : Effects)
			{
				Apply(ASC, GE);
			}
			return ASC;
		}

		static void Apply(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> GE)
		{
			ASC->ApplyGameplayEffectToSelf(GE->GetDefaultObject<UGameplayEffect>(), 1.f, ASC->MakeEffectContext());
		}

		static void Hit(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, float MotionValue)
		{
			FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Source->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_MotionValue, MotionValue);
			Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target);
		}

		static void FixedHit(UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, TSubclassOf<UGameplayEffect> GE, float FixedDamage)
		{
			FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(GE, 1.f, Source->MakeEffectContext());
			Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_FixedDamage, FixedDamage);
			Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, Target);
		}

		static float HP(const UAbilitySystemComponent* ASC)
		{
			return ASC->GetNumericAttribute(UMetaCombatAttributeSet::GetHealthAttribute());
		}

		static float D(const UAbilitySystemComponent* ASC)
		{
			return ASC->GetNumericAttribute(UMetaCombatAttributeSet::GetAttackPowerAttribute());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatArmoredDRTest, "AccessedNone.Combat.ArmoredDR", CombatTestFlags)
bool FCombatArmoredDRTest::RunTest(const FString& Parameters)
{
	FCombatFixture F;
	UAbilitySystemComponent* Player = F.SpawnKnight();
	TestEqual(TEXT("knight D"), F.D(Player), 100.f);
	UAbilitySystemComponent* Boss = F.SpawnBoss(F.D(Player), 10.f, { UGE_Boss_Armored::StaticClass() });
	TestTrue(TEXT("State.Armored granted"), Boss->HasMatchingGameplayTag(MetaTags::State_Armored));

	F.Hit(Player, Boss, 1.f);
	TestNearlyEqual(TEXT("raw 100 -> 1 damage"), F.HP(Boss), 999.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatInvulnerableTest, "AccessedNone.Combat.Invulnerable", CombatTestFlags)
bool FCombatInvulnerableTest::RunTest(const FString& Parameters)
{
	FCombatFixture F;
	UAbilitySystemComponent* Player = F.SpawnKnight();
	UAbilitySystemComponent* Boss = F.SpawnBoss(F.D(Player), 10.f, { UGE_Boss_Invulnerable::StaticClass() });

	int32 ImmuneEvents = 0;
	Boss->GenericGameplayEventCallbacks.FindOrAdd(MetaTags::Event_Hit_Immune).AddLambda([&ImmuneEvents](const FGameplayEventData*) { ++ImmuneEvents; });

	F.Hit(Player, Boss, 1.f);
	F.FixedHit(Player, Boss, UGE_InstantKill::StaticClass(), 0.f);
	TestEqual(TEXT("no damage"), F.HP(Boss), 1000.f);
	TestEqual(TEXT("immune event per hit"), ImmuneEvents, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatReflectTest, "AccessedNone.Combat.Reflect", CombatTestFlags)
bool FCombatReflectTest::RunTest(const FString& Parameters)
{
	{
		FCombatFixture F;
		UAbilitySystemComponent* Player = F.SpawnKnight();
		// Player also reflects: proves a reflect hit is never reflected again.
		F.Apply(Player, UGE_Boss_Reflect::StaticClass());
		UAbilitySystemComponent* Boss = F.SpawnBoss(F.D(Player), 10.f, { UGE_Boss_Reflect::StaticClass() });

		F.Hit(Player, Boss, 1.f);
		TestEqual(TEXT("boss takes raw 100"), F.HP(Boss), 900.f);
		TestEqual(TEXT("attacker takes 300"), F.HP(Player), 700.f);
	}
	{
		// E2: armored + reflect. Reflect uses raw, not reduced damage.
		FCombatFixture F;
		UAbilitySystemComponent* Player = F.SpawnKnight();
		UAbilitySystemComponent* Boss = F.SpawnBoss(F.D(Player), 10.f, { UGE_Boss_Armored::StaticClass(), UGE_Boss_Reflect::StaticClass() });

		F.Hit(Player, Boss, 1.f);
		TestNearlyEqual(TEXT("E2 boss takes 1"), F.HP(Boss), 999.f, 0.01f);
		TestEqual(TEXT("E2 attacker takes 300"), F.HP(Player), 700.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatIFrameTest, "AccessedNone.Combat.IFrame", CombatTestFlags)
bool FCombatIFrameTest::RunTest(const FString& Parameters)
{
	FCombatFixture F;
	UAbilitySystemComponent* Boss = F.SpawnKnight();
	UAbilitySystemComponent* Player = F.SpawnKnight();
	F.Apply(Player, UGE_IFrame::StaticClass());
	TestTrue(TEXT("State.IFrame granted"), Player->HasMatchingGameplayTag(MetaTags::State_IFrame));

	int32 DeathEvents = 0;
	Player->GenericGameplayEventCallbacks.FindOrAdd(MetaTags::Event_Death).AddLambda([&DeathEvents](const FGameplayEventData*) { ++DeathEvents; });

	F.FixedHit(Boss, Player, UGE_Damage::StaticClass(), 600.f);
	TestEqual(TEXT("normal hit ignored during i-frame"), F.HP(Player), 1000.f);

	F.FixedHit(Boss, Player, UGE_InstantKill::StaticClass(), 0.f);
	TestEqual(TEXT("IgnoreIFrame instant kill applies"), F.HP(Player), 0.f);
	TestEqual(TEXT("one death event"), DeathEvents, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatHitsToKillTest, "AccessedNone.Combat.HitsToKill", CombatTestFlags)
bool FCombatHitsToKillTest::RunTest(const FString& Parameters)
{
	auto HitsToKill = [](float MotionValue)
	{
		FCombatFixture F;
		UAbilitySystemComponent* Player = F.SpawnKnight();
		UAbilitySystemComponent* Boss = F.SpawnBoss(F.D(Player), 10.f, { UGE_Boss_Armored::StaticClass() });
		Boss->RemoveActiveGameplayEffectBySourceEffect(UGE_Boss_Armored::StaticClass(), nullptr);
		FCombatFixture::Apply(Boss, UGE_Boss_Vulnerable::StaticClass());

		int32 Hits = 0;
		while (F.HP(Boss) > 0.f && Hits < 100)
		{
			F.Hit(Player, Boss, MotionValue);
			++Hits;
		}
		return Hits;
	};

	TestEqual(TEXT("light attacks to kill vulnerable boss"), HitsToKill(1.f), 10);
	TestEqual(TEXT("heavy attacks to kill vulnerable boss"), HitsToKill(1.5f), 7);
	return true;
}

#endif
