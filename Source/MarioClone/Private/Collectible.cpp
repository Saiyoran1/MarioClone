#include "Collectible.h"

#include "MarioPlayerCharacter.h"
#include "PaperSpriteComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

const FName ACollectible::CollectibleProfile = FName(TEXT("Collectible"));

ACollectible::ACollectible()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(FName(TEXT("CollisionSphere")));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(FName(TEXT("Sprite")));
	Sprite->SetupAttachment(CollisionSphere);
	Sprite->SetUsingAbsoluteRotation(true);
	Sprite->SetWorldRotation(FRotator(0.0f));
}

void ACollectible::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACollectible, bCollected);
}

void ACollectible::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (IsValid(CollisionSphere))
		{
			CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectible::OnOverlap);
			CollisionSphere->SetCollisionProfileName(CollectibleProfile);
		}
		GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
		if (IsValid(GameStateRef))
		{
			GameStartCallback.BindDynamic(this, &ACollectible::OnGameStarted);
			GameStateRef->SubscribeToGameStarted(GameStartCallback);
		}
		else
		{
			GameStateDelegateHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ACollectible::OnGameStateSet);
		}
	}
}

void ACollectible::OnGameStateSet(AGameStateBase* GameState)
{
	GameStateRef = Cast<AMarioGameState>(GameState);
	if (IsValid(GameStateRef))
	{
		GetWorld()->GameStateSetEvent.Remove(GameStateDelegateHandle);
		GameStartCallback.BindDynamic(this, &ACollectible::OnGameStarted);
		GameStateRef->SubscribeToGameStarted(GameStartCallback);
	}
}

void ACollectible::OnGameStarted()
{
	if (bCollected)
	{
		if (IsValid(CollisionSphere))
		{
			CollisionSphere->SetCollisionProfileName(CollectibleProfile);
		}
		bCollected = false;
		OnRep_bCollected();
	}
}

void ACollectible::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AMarioPlayerCharacter* OverlappingPlayer = Cast<AMarioPlayerCharacter>(OtherActor);
	if (IsValid(OverlappingPlayer))
	{
		OverlappingPlayer->GrantCollectible(CollectibleValue);
		bCollected = true;
		if (IsValid(CollisionSphere))
		{
			CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		OnRep_bCollected();
	}
}

void ACollectible::OnRep_bCollected()
{
	if (IsValid(Sprite))
	{
		Sprite->SetVisibility(!bCollected);
	}
}