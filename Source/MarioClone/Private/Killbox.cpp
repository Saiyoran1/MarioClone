#include "Killbox.h"

#include "CombatInterface.h"
#include "Components/BoxComponent.h"

const FName AKillbox::KillBoxProfile = FName(TEXT("Killbox"));

AKillbox::AKillbox()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionProfileName(KillBoxProfile);
}

void AKillbox::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_Client)
	{
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AKillbox::OnCollision);
	}
}

void AKillbox::OnCollision(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_InstantKill(OtherActor);
	}
}