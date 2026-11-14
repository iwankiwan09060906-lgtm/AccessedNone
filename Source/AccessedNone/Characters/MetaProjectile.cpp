#include "Characters/MetaProjectile.h"
#include "Characters/MetaCharacterBase.h"
#include "Combat/MetaGameplayEffects.h"
#include "Core/MetaGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

AMetaProjectile::AMetaProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(14.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Trail = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail"));
	Trail->SetupAttachment(RootComponent);
	Trail->bAutoActivate = false;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;
}

void AMetaProjectile::Launch(UAbilitySystemComponent* InSourceASC, const FVector& Direction, float Speed, float InMotionValue, FGameplayTag InDamageType,
	UParticleSystem* TrailFX, UParticleSystem* InHitFX)
{
	SourceASC = InSourceASC;
	MotionValue = InMotionValue;
	DamageType = InDamageType;
	HitFX = InHitFX;
	if (AActor* Shooter = InSourceASC ? InSourceASC->GetAvatarActor() : nullptr)
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AMetaProjectile::HandleOverlap);
	Collision->OnComponentHit.AddDynamic(this, &AMetaProjectile::HandleHit);
	SetActorRotation(Direction.Rotation());
	if (TrailFX)
	{
		Trail->SetTemplate(TrailFX);
		Trail->Activate(true);
	}
	Movement->Velocity = Direction.GetSafeNormal() * Speed;
	Movement->Activate(true);
	SetLifeSpan(LifeSeconds);
}

void AMetaProjectile::HandleOverlap(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	const AMetaCharacterBase* Target = Cast<AMetaCharacterBase>(Other);
	UAbilitySystemComponent* Source = SourceASC.Get();
	if (!Target || !Source || Other == Source->GetAvatarActor() || Target->IsDead() || Target->IsHidden())
	{
		return;
	}
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Other))
	{
		FGameplayEffectSpecHandle Spec = Source->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Source->MakeEffectContext());
		Spec.Data->SetSetByCallerMagnitude(MetaTags::Data_MotionValue, MotionValue);
		Spec.Data->AddDynamicAssetTag(DamageType);
		Source->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
	Impact(GetActorLocation());
}

void AMetaProjectile::HandleHit(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, FVector Impulse, const FHitResult& Hit)
{
	Impact(Hit.ImpactPoint);
}

void AMetaProjectile::Impact(const FVector& Location)
{
	if (HitFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, HitFX, Location, GetActorRotation());
	}
	Destroy();
}
