#include "Core/BreakpointActor.h"
#include "Characters/MetaPlayerCharacter.h"
#include "Combat/MetaCombatAttributeSet.h"
#include "Core/MetaGameMode.h"
#include "Core/MetaPlayerController.h"
#include "Core/MetaText.h"
#include "Meta/EncounterSubsystem.h"
#include "Meta/MetaSaveSubsystem.h"
#include "UI/MetaHUDWidget.h"
#include "UI/MetaUI.h"
#include "AbilitySystemComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float OrbHeight = 110.f;
	constexpr float RespawnDistance = 180.f;
}

ABreakpointActor::ABreakpointActor()
{
	PrimaryActorTick.bCanEverTick = true;
	Orb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Orb"));
	RootComponent = Orb;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Orb->SetStaticMesh(Sphere.Object);
	}
	Orb->SetRelativeScale3D(FVector(0.6f));
	Orb->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetLightColor(MetaUI::Error);
	Glow->SetIntensity(40.f);
	Glow->SetAttenuationRadius(600.f);
}

void ABreakpointActor::BeginPlay()
{
	Super::BeginPlay();
	AddActorWorldOffset(FVector(0.f, 0.f, OrbHeight)); // placed on the ground, floats at chest height
	BaseZ = GetActorLocation().Z;
	if (UMaterialInstanceDynamic* MID = Orb->CreateDynamicMaterialInstance(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), MetaUI::Error); // engine BasicShapeMaterial parameter
	}
}

void ABreakpointActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float T = GetWorld()->GetTimeSeconds();
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, BaseZ + 8.f * FMath::Sin(T * 1.5f)));
	Glow->SetIntensity(30.f + 15.f * FMath::Sin(T * 2.f));
}

FTransform ABreakpointActor::GetRespawnTransform() const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	FVector Loc = GetActorLocation() + Forward * RespawnDistance;
	Loc.Z = BaseZ - OrbHeight + 100.f;
	return FTransform(Forward.Rotation(), Loc);
}

FText ABreakpointActor::GetInteractPrompt() const
{
	const UEncounterSubsystem* Encounters = GetWorld()->GetSubsystem<UEncounterSubsystem>();
	return Encounters && Encounters->IsInCombatEncounter() ? FText::GetEmpty() : MetaText::Get(TEXT("UI_BREAKPOINT_PROMPT"));
}

void ABreakpointActor::Interact(AMetaPlayerCharacter* Player)
{
	if (!Player || GetInteractPrompt().IsEmpty())
	{
		return; // spec 6.2: no saving during an encounter
	}
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->SetNumericAttributeBase(UMetaCombatAttributeSet::GetHealthAttribute(), Player->GetMaxHealth());
	Player->RefillHealFlasks();
	if (AMetaGameMode* GM = GetWorld()->GetAuthGameMode<AMetaGameMode>())
	{
		GM->SetActiveBreakpoint(this);
	}
	GetGameInstance()->GetSubsystem<UMetaSaveSubsystem>()->SaveAtBreakpoint(BreakpointId, GetWorld());
	if (const AMetaPlayerController* PC = Cast<AMetaPlayerController>(Player->GetController()); PC && PC->GetHUDWidget())
	{
		PC->GetHUDWidget()->ShowToast(MetaText::Get(TEXT("UI_BREAKPOINT_SAVE")), 2.f, MetaUI::Gold);
	}
}
