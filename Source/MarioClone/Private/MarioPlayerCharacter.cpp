#include "MarioClone/Public/MarioPlayerCharacter.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"

#pragma region Core

AMarioPlayerCharacter::AMarioPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("Camera")));
	Camera->SetupAttachment(RootComponent);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetWorldRotation(FRotator(0.0f));
	Camera->SetRelativeLocation(FVector(0.0f, 800.0f, 100.0f));

	PlayerHitbox = CreateDefaultSubobject<UHitbox>(FName(TEXT("Hitbox")));
	PlayerHitbox->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	HealthComponent->SetupAttachment(RootComponent);
}

void AMarioPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	//Constrain movement to the Y and Z axes, since this is a 2D game.
	GetMovementComponent()->SetPlaneConstraintEnabled(true);
	GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::X);

	//Bind to the health and life status delegates of the health component.
	HealthCallback.BindDynamic(this, &AMarioPlayerCharacter::OnHealthChanged);
	LifeCallback.BindDynamic(this, &AMarioPlayerCharacter::OnLifeStatusChanged);
	HealthComponent->SubscribeToHealthChanged(HealthCallback);
	HealthComponent->SubscribeToLifeStatusChanged(LifeCallback);

	HitboxCallback.BindDynamic(this, &AMarioPlayerCharacter::OnHitboxCollision);
	PlayerHitbox->SubscribeToHitboxCollision(HitboxCallback);

	if (IsLocallyControlled())
	{
		//Cache off camera's initial world space offset, so that we can use it as a reference point when leading the player.
		DesiredBaseCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	}
}

void AMarioPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("Movement"), this, &AMarioPlayerCharacter::MovementInput);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &AMarioPlayerCharacter::JumpPressed);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Released, this, &AMarioPlayerCharacter::JumpReleased);

#if WITH_EDITOR
	//Debug keybindings for testing
	PlayerInputComponent->BindAction(FName("DEBUG_DamageSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_DamageSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_HealSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_HealSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_KillSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_KillSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_ReviveSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_ReviveSelf);
#endif
}

void AMarioPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (Controller.Get() && Controller->IsLocalPlayerController())
	{
		AMarioGameState* GameState = Cast<AMarioGameState>(GetWorld()->GetGameState());
		if (IsValid(GameState))
		{
			GameState->InitializePlayer(this);
		}
		else
		{
			GameStateDelegateHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &AMarioPlayerCharacter::OnGameStateSet);
		}
	}
}

void AMarioPlayerCharacter::OnGameStateSet(AGameStateBase* GameState)
{
	AMarioGameState* MarioGameState = Cast<AMarioGameState>(GetWorld()->GetGameState());
	if (IsValid(MarioGameState))
	{
		GetWorld()->GameStateSetEvent.Remove(GameStateDelegateHandle);
		MarioGameState->InitializePlayer(this);
	}
}

void AMarioPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		//Update camera location to lead the character when moving.
		const float LastMoveInputY = GetMovementComponent()->GetLastInputVector().Y;
		//If we aren't moving, we'll update how long we haven't been moving for.
		if (LastMoveInputY == 0.0f)
		{
			TimeAtRest += DeltaTime;
			//After a delay, we'll recenter the camera on the character.
			if (TimeAtRest > CameraRecenterDelay)
			{
				Camera->SetWorldLocation(FMath::VInterpConstantTo(Camera->GetComponentLocation(), GetActorLocation() + DesiredBaseCameraOffset, DeltaTime, CameraRecenterSpeed));
			}
		}
		//If we are moving, the camera should lead the character along the Y axis.
		else
		{
			TimeAtRest = 0.0f;
			const FVector DesiredOffset = FVector(DesiredBaseCameraOffset.X, DesiredBaseCameraOffset.Y + (LastMoveInputY * MaxCameraLeadOffset), DesiredBaseCameraOffset.Z);
			const FVector DesiredFinalLocation = GetActorLocation() + DesiredOffset;
			Camera->SetWorldLocation(FMath::VInterpConstantTo(Camera->GetComponentLocation(), DesiredFinalLocation, DeltaTime, CameraLeadInterpSpeed));
		}
	}

	DrawDebugString(GetWorld(), GetActorLocation() + FVector::UpVector * 100.0f,
		FString::Printf(L"%i/%i", FMath::RoundToInt(HealthComponent->GetCurrentHealth()), FMath::RoundToInt(HealthComponent->GetMaxHealth())), 0, FColor::Red, 0);
}

#pragma endregion 
#pragma region Movement

void AMarioPlayerCharacter::MovementInput(const float AxisValue)
{
	GetMovementComponent()->AddInputVector(FVector(0.0f, 1.0f, 0.0f) * AxisValue);
}

void AMarioPlayerCharacter::JumpPressed()
{
	Jump();
}

void AMarioPlayerCharacter::JumpReleased()
{
	//TODO: Float/mana/drop attack.
}

#pragma endregion 
#pragma region Health

void AMarioPlayerCharacter::OnHealthChanged(const float PreviousHealth, const float CurrentHealth, const float MaxHealth)
{
	//If we took damage, we need to reset our health regen timer and become briefly immune to damage.
	if (CurrentHealth < PreviousHealth)
	{
		//TODO: Server health regeneration
		//TODO: All machines, play a hit reaction (if we didn't hit 0 health here).
	}
}

void AMarioPlayerCharacter::OnLifeStatusChanged(const bool bNewLifeStatus)
{
	//TODO: Play death animation, stop input, timer for respawn?
}

#pragma endregion
#pragma region Collision

void AMarioPlayerCharacter::OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue)
{
	if (IsLocallyControlled() && BounceImpulse != FVector::ZeroVector)
	{
		LaunchCharacter(BounceImpulse, false, true);
	}
	if (HasAuthority() && IsValid(HealthComponent) && DamageValue != 0.0f)
	{
		HealthComponent->ModifyHealth(DamageValue * -1.0f);
	}
}

#pragma endregion 

#if WITH_EDITOR
#pragma region Debug

void AMarioPlayerCharacter::DEBUG_DamageSelf()
{
	Server_DEBUG_DamageSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_DamageSelf_Implementation()
{
	HealthComponent->ModifyHealth(-5.0f);
}

void AMarioPlayerCharacter::DEBUG_HealSelf()
{
	Server_DEBUG_HealSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_HealSelf_Implementation()
{
	HealthComponent->ModifyHealth(3.5f);
}

void AMarioPlayerCharacter::DEBUG_KillSelf()
{
	Server_DEBUG_KillSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_KillSelf_Implementation()
{
	HealthComponent->InstantKill();
}

void AMarioPlayerCharacter::DEBUG_ReviveSelf()
{
	Server_DEBUG_ReviveSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_ReviveSelf_Implementation()
{
	HealthComponent->ResetHealth();
}

#pragma endregion
#endif