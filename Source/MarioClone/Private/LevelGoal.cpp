#include "LevelGoal.h"

#include "MarioPlayerCharacter.h"
#include "Components/SphereComponent.h"

FName ALevelGoal::GoalCollisionProfile = FName(TEXT("LevelGoal"));

ALevelGoal::ALevelGoal()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<USphereComponent>(FName(TEXT("CollisionBox")));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionProfileName(GoalCollisionProfile);
}

void ALevelGoal::BeginPlay()
{
	Super::BeginPlay();

	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ALevelGoal::OnOverlap);
}

void ALevelGoal::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}
	const AMarioPlayerCharacter* OverlappingPlayer = Cast<AMarioPlayerCharacter>(OtherActor);
	if (IsValid(OverlappingPlayer))
	{
		AMarioGameState* GameState = Cast<AMarioGameState>(GetWorld()->GetGameState());
		if (IsValid(GameState))
		{
			GameState->GoalReached();
		}
	}
}
