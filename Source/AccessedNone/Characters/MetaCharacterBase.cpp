#include "Characters/MetaCharacterBase.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/MetaGameplayTags.h"
#include "Core/MetaLog.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AMetaCharacterBase::AMetaCharacterBase()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	// Default subobject of the actor → the ASC discovers it automatically.
	Attributes = CreateDefaultSubobject<UMetaCombatAttributeSet>(TEXT("Attributes"));
}

void AMetaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (InitEffect)
	{
		AbilitySystem->ApplyGameplayEffectToSelf(InitEffect->GetDefaultObject<UGameplayEffect>(), 1.f, AbilitySystem->MakeEffectContext());
	}
	AbilitySystem->GenericGameplayEventCallbacks.FindOrAdd(MetaTags::Event_Death).AddUObject(this, &AMetaCharacterBase::HandleDeath);
}

float AMetaCharacterBase::GetHealth() const { return Attributes->GetHealth(); }
float AMetaCharacterBase::GetMaxHealth() const { return Attributes->GetMaxHealth(); }
bool AMetaCharacterBase::IsDead() const { return AbilitySystem->HasMatchingGameplayTag(MetaTags::State_Dead); }

void AMetaCharacterBase::HandleDeath(const FGameplayEventData* /*Payload*/)
{
	if (IsDead())
	{
		return;
	}
	UE_LOG(LogMetaCombat, Log, TEXT("%s died"), *GetName());
	AbilitySystem->AddLooseGameplayTag(MetaTags::State_Dead);
	AbilitySystem->CancelAllAbilities();
	GetCharacterMovement()->DisableMovement();
	OnDeath();
}
