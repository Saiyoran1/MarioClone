#include "NPCCharacter.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperFlipbookComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"

const FName ANPCCharacter::BumperProfile = FName(TEXT("Bumper"));

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetWorldRotation(FRotator(0.0f));
	
	Hitbox = CreateDefaultSubobject<UHitbox>(FName(TEXT("Hitbox")));
	Hitbox->SetupAttachment(RootComponent);
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	HealthComponent->SetupAttachment(RootComponent);
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CachedStartLocation = GetActorLocation();
		CachedStartRotation = GetActorRotation();
	}
	
	//Constrain movement to the X and Z axes, since this is a 2D game.
	GetMovementComponent()->SetPlaneConstraintEnabled(true);
	GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);

	HitboxCallback.BindDynamic(this, &ANPCCharacter::OnHitboxCollision);
	Hitbox->SubscribeToHitboxCollision(HitboxCallback);

	LifeCallback.BindDynamic(this, &ANPCCharacter::OnLifeStatusChanged);
	HealthComponent->SubscribeToLifeStatusChanged(LifeCallback);

	GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
	if (IsValid(GameStateRef))
	{
		BindToGameStartEnd();
	}
	else
	{
		GameStateDelegateHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &ANPCCharacter::OnGameStateSet);
	}
}

void ANPCCharacter::OnGameStateSet(AGameStateBase* GameState)
{
	GameStateRef = Cast<AMarioGameState>(GameState);
	if (IsValid(GameStateRef))
	{
		GetWorld()->GameStateSetEvent.Remove(GameStateDelegateHandle);
		BindToGameStartEnd();
	}
}

void ANPCCharacter::BindToGameStartEnd()
{
	GameStartCallback.BindDynamic(this, &ANPCCharacter::OnGameStart);
	GameEndCallback.BindDynamic(this, &ANPCCharacter::OnGameEnd);
	GameStateRef->SubscribeToGameStarted(GameStartCallback);
	GameStateRef->SubscribeToGameEnded(GameEndCallback);
	if (GameStateRef->HasGameEnded())
	{
		DisableNPC();
	}
}

void ANPCCharacter::OnGameStart()
{
	if (HasAuthority())
	{
		TeleportTo(CachedStartLocation, CachedStartRotation);
		HealthComponent->ResetHealth();
	}
	EnableNPC();
}

void ANPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsEnabled)
	{
		return;
	}
	if (IsValid(HealthComponent) && !HealthComponent->IsAlive())
	{
		return;
	}

	if (HasAuthority())
	{
		if (bShouldMove)
		{
			//Some enemies can randomly jump to make them more interesting.
			if (bShouldRandomlyJump)
			{
				TimeTilJumpAttempt -= DeltaTime;
				if (TimeTilJumpAttempt < 0.0f)
				{
					if (FMath::FRand() <= RandomJumpChance)
					{
						Jump();
					}
					TimeTilJumpAttempt = RandomJumpInterval + FMath::RandRange(-1.0f * RandomJumpIntervalVariance, RandomJumpIntervalVariance);
				}
			}
			//Don't try to turn around in the air.
			if (GetCharacterMovement()->IsMovingOnGround())
			{
				//Check if there are any walls in front of the actor.
				FHitResult HorizontalHit;
				const FVector TraceStart = GetCapsuleComponent()->GetComponentLocation() + FVector(GetCapsuleComponent()->GetScaledCapsuleRadius() * bWasMovingForward ? 1.0f : -1.0f, 0.0f, 0.0f);
				const FVector TraceEnd = TraceStart + (FVector(bWasMovingForward ? 1.0f : -1.0f, 0.0f, 0.0f) * BumperDistanceHorizontal);
				if (GetWorld()->LineTraceSingleByProfile(HorizontalHit, TraceStart, TraceEnd, BumperProfile))
				{
					bWasMovingForward = !bWasMovingForward;
				}
				//If no walls, check that there is valid floor in front of the actor.
				else
				{
					FHitResult VerticalHit;
					const FVector VertTraceStart = TraceEnd;
					const FVector VertTraceEnd = VertTraceStart + (FVector::DownVector * BumperDistanceVertical);
					if (!GetWorld()->LineTraceSingleByProfile(VerticalHit, VertTraceStart, VertTraceEnd, BumperProfile))
					{
						bWasMovingForward = !bWasMovingForward;
					}
				}
			}
			GetCharacterMovement()->AddInputVector(FVector(bWasMovingForward ? 1.0f : -1.0f, 0.0f, 0.0f));
		}
	}
}

void ANPCCharacter::InstantKill_Implementation()
{
	if (IsValid(HealthComponent))
	{
		HealthComponent->InstantKill();
	}
}

void ANPCCharacter::EnableNPC()
{
	if (bIsEnabled)
	{
		return;
	}
	
	SetActorTickEnabled(true);
	if (IsValid(Hitbox))
	{
		Hitbox->EnableHitbox();
	}
	if (IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}

	bIsEnabled = true;
}

void ANPCCharacter::DisableNPC()
{
	if (!bIsEnabled)
	{
		return;
	}
	
	SetActorTickEnabled(false);
	if (IsValid(Hitbox))
	{
		Hitbox->DisableHitbox();
	}
	if (IsValid(GetCharacterMovement()))
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	bIsEnabled = false;
}

void ANPCCharacter::OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue)
{
	if (HasAuthority())
	{
		if (!BounceImpulse.IsNearlyZero())
		{
			LaunchCharacter(BounceImpulse, false, true);
		}
		if (DamageValue != 0.0f)
		{
			HealthComponent->ModifyHealth(DamageValue * -1.0f);
		}
	}
}

void ANPCCharacter::OnLifeStatusChanged(const bool bNewLifeStatus)
{
	if (bNewLifeStatus)
	{
		EnableNPC();
		GetSprite()->SetVisibility(true);
	}
	else
	{
		DisableNPC();
		GetSprite()->SetVisibility(false);
	}
}



