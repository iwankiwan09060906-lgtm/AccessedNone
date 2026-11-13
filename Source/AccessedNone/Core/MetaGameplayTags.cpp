#include "Core/MetaGameplayTags.h"

namespace MetaTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Armored, "State.Armored");
	UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_Vulnerable, "State.Vulnerable");
	UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
	UE_DEFINE_GAMEPLAY_TAG(State_Breaking, "State.Breaking");
	UE_DEFINE_GAMEPLAY_TAG(State_IFrame, "State.IFrame");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Reflect, "Gimmick.Reflect");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_Stealth, "Gimmick.Stealth");
	UE_DEFINE_GAMEPLAY_TAG(Gimmick_SoundBound, "Gimmick.SoundBound");

	UE_DEFINE_GAMEPLAY_TAG(Damage_IgnoreIFrame, "Damage.IgnoreIFrame");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Light, "Damage.Type.Light");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Heavy, "Damage.Type.Heavy");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Skill, "Damage.Type.Skill");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_InstantKill, "Damage.Type.InstantKill");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Reflect, "Damage.Type.Reflect");

	UE_DEFINE_GAMEPLAY_TAG(Bounded_InMap, "Bounded.InMap");

	UE_DEFINE_GAMEPLAY_TAG(Data_MotionValue, "Data.MotionValue");
	UE_DEFINE_GAMEPLAY_TAG(Data_FixedDamage, "Data.FixedDamage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Duration, "Data.Duration");

	UE_DEFINE_GAMEPLAY_TAG(Ability_Player_LightAttack, "Ability.Player.LightAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Player_HeavyAttack, "Ability.Player.HeavyAttack");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Player_Dodge, "Ability.Player.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Player_Heal, "Ability.Player.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Player_ThrowDagger, "Ability.Player.ThrowDagger");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Boss_Annihilate, "Ability.Boss.Annihilate");

	UE_DEFINE_GAMEPLAY_TAG(Event_Death, "Event.Death");
	UE_DEFINE_GAMEPLAY_TAG(Event_Gimmick_Broken, "Event.Gimmick.Broken");
	UE_DEFINE_GAMEPLAY_TAG(Event_Encounter_Begin, "Event.Encounter.Begin");
	UE_DEFINE_GAMEPLAY_TAG(Event_Encounter_End, "Event.Encounter.End");
	UE_DEFINE_GAMEPLAY_TAG(Event_Hit_Immune, "Event.Hit.Immune");
}
