﻿#include "MarioClone/Public/MarioPlayerCharacter.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "MarioMovementComponent.h"
#include "PaperFlipbookComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UI/PlayerHUD.h"

#pragma region Core

AMarioPlayerCharacter::AMarioPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass(CharacterMovementComponentName, UMarioMovementComponent::StaticClass()))
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("Camera")));
	Camera->SetupAttachment(RootComponent);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetWorldRotation(FRotator(0.0f, -90.0f, 0.0f));
	Camera->SetRelativeLocation(FVector(0.0f, 800.0f, 100.0f));

	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetWorldRotation(FRotator(0.0f));

	PlayerHitbox = CreateDefaultSubobject<UHitbox>(FName(TEXT("Hitbox")));
	PlayerHitbox->SetupAttachment(RootComponent);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	HealthComponent->SetupAttachment(RootComponent);

	MarioMoveComponent = Cast<UMarioMovementComponent>(GetCharacterMovement());
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

void AMarioPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentLives = MaxLives;
	OnLivesChanged.Broadcast(CurrentLives);
	RespawnDelay = FMath::Max(0.0f, RespawnDelay);
	CachedSpawnLocation = GetActorLocation();
	CachedSpawnRotation = GetActorRotation();
	
	//Constrain movement to the X and Z axes, since this is a 2D game.
	GetMovementComponent()->SetPlaneConstraintEnabled(true);
	GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);

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

	//Subscribe to the GameState's GameStart and GameEnd delegates.
	if (HasAuthority() || IsLocallyControlled())
	{
		if (!IsValid(GameStateRef))
		{
			GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
		}
		if (IsValid(GameStateRef))
		{
			GameStartCallback.BindDynamic(this, &AMarioPlayerCharacter::OnGameStarted);
			GameStateRef->SubscribeToGameStarted(GameStartCallback);
			GameEndCallback.BindDynamic(this, &AMarioPlayerCharacter::OnGameEnded);
			GameStateRef->SubscribeToGameEnded(GameEndCallback);
		}
	}
}

void AMarioPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//If the player is dead, we can early exit here and not try to adjust our sprite.
	if (IsValid(HealthComponent) && !HealthComponent->IsAlive())
	{
		return;
	}
	
	UPaperFlipbook* Flipbook = nullptr;
	FVector Velocity = FVector::ZeroVector;
	if (IsValid(MarioMoveComponent))
	{
		Velocity = MarioMoveComponent->GetLastUpdateVelocity();
		if (MarioMoveComponent->IsFalling())
		{
			Flipbook = JumpingFlipbook;
		}
		else
		{
			Flipbook = Velocity.IsNearlyZero() ? IdleFlipbook : RunningFlipbook;
		}
	}

	//Fallback in case all flipbook was null or our movement component was null.
	if (!IsValid(Flipbook))
	{
		if (IsValid(IdleFlipbook))
		{
			Flipbook = IdleFlipbook;
		}
		else if (IsValid(RunningFlipbook))
		{
			Flipbook = RunningFlipbook;
		}
		else if (IsValid(JumpingFlipbook))
		{
			Flipbook = JumpingFlipbook;
		}
	}
	//Final check for null, if we don't have any valid flipbooks at all, do nothing.
	if (IsValid(Flipbook))
	{
		GetSprite()->SetFlipbook(Flipbook);
	}
	
	//Check to see whether we should flip our sprite around to move left or right.
	if (Velocity.X != 0.0f)
	{
		const bool bMovingRight = Velocity.X > 0.0f;
		if (bMovingRight != bWasMovingRight)
		{
			bWasMovingRight = bMovingRight;
			GetSprite()->SetWorldRotation(FRotator(0.0f, bMovingRight ? 0.0f : 180.0f, 0.0f));
		}
	}
}

void AMarioPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (Controller.Get() && Controller->IsLocalPlayerController())
	{
		PlayerController = Cast<APlayerController>(Controller.Get());
		GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
		if (IsValid(GameStateRef))
		{
			GameStateRef->InitializePlayer(this);
		}
		else
		{
			GameStateDelegateHandle = GetWorld()->GameStateSetEvent.AddUObject(this, &AMarioPlayerCharacter::OnGameStateSet);
		}
	}
	if (!IsValid(HUD) && IsValid(PlayerController) && IsValid(HUDClass))
	{
		HUD = CreateWidget<UPlayerHUD>(PlayerController, HUDClass);
		if (IsValid(HUD))
		{
			HUD->AddToViewport();
			HUD->Init(this);
		}
	}
}

void AMarioPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMarioPlayerCharacter, CurrentLives, COND_OwnerOnly);
}

void AMarioPlayerCharacter::OnGameStateSet(AGameStateBase* GameState)
{
	GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
	if (IsValid(GameStateRef))
	{
		GetWorld()->GameStateSetEvent.Remove(GameStateDelegateHandle);
		GameStateRef->InitializePlayer(this);
	}
}

void AMarioPlayerCharacter::OnGameStarted()
{
	if (HasAuthority())
	{
		CurrentLives = MaxLives;
		OnRep_CurrentLives();
		Respawn();
	}
	if (!bIsEnabled)
	{
		EnablePlayer();
	}
	if (IsLocallyControlled() && IsValid(GameOverScreen))
	{
		GameOverScreen->RemoveFromParent();
		GameOverScreen = nullptr;
		if (IsValid(PlayerController))
		{
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void AMarioPlayerCharacter::OnGameEnded(const bool bGameWon)
{
	DisablePlayer();
	if (IsLocallyControlled() && IsValid(PlayerController) && !IsValid(GameOverScreen) && IsValid(GameOverScreenClass))
	{
		GameOverScreen = CreateWidget<UGameOverScreen>(PlayerController, GameOverScreenClass);
		if (IsValid(GameOverScreen))
		{
			GameOverScreen->AddToViewport();
			PlayerController->SetInputMode(FInputModeUIOnly());
			PlayerController->SetShowMouseCursor(true);
			if (!RestartCallback.IsBound())
			{
				RestartCallback.BindDynamic(this, &AMarioPlayerCharacter::AMarioPlayerCharacter::RestartRequested);
			}
			GameOverScreen->Init(bGameWon, RestartCallback);
		}
	}
}

void AMarioPlayerCharacter::Server_RequestRestart_Implementation()
{
	if (!IsValid(GameStateRef))
	{
		GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
	}
	if (IsValid(GameStateRef))
	{
		GameStateRef->RequestRestartGame();
	}
}

void AMarioPlayerCharacter::EnablePlayer()
{
	//Reset flipbook and rotation.
	if (IsValid(GetSprite()))
	{
		if (IsValid(IdleFlipbook))
		{
			GetSprite()->SetFlipbook(IdleFlipbook);
		}
		GetSprite()->SetWorldRotation(FRotator(0.0f, 0.0f, 0.0f));
		bWasMovingRight = true;
	}
	//Re-enable movement.
	if (IsValid(MarioMoveComponent))
	{
		MarioMoveComponent->SetMovementMode(MOVE_Falling);
	}
	//Re-enable input
	if (IsLocallyControlled() && IsValid(PlayerController))
	{
		EnableInput(PlayerController);
	}
	//Re-enable hitbox collision
	if (IsValid(PlayerHitbox))
	{
		PlayerHitbox->EnableHitbox();
	}

	bIsEnabled = true;
}

void AMarioPlayerCharacter::DisablePlayer()
{
	//Discount death pose.
	if (IsValid(GetSprite()))
	{
		if (IsValid(IdleFlipbook))
		{
			GetSprite()->SetFlipbook(IdleFlipbook);
		}
		GetSprite()->SetWorldRotation(FRotator(90.0f, GetSprite()->GetComponentRotation().Yaw, 0.0f));
	}
	//Disable movement.
	if (IsValid(MarioMoveComponent))
	{
		MarioMoveComponent->DisableMovement();
	}
	//Disable input.
	if (IsLocallyControlled() && IsValid(PlayerController))
	{
		DisableInput(PlayerController);
	}
	//Disable hitbox collision.
	if (IsValid(PlayerHitbox))
	{
		PlayerHitbox->DisableHitbox();
	}

	bIsEnabled = false;
}

#pragma endregion 
#pragma region Movement

void AMarioPlayerCharacter::MovementInput(const float AxisValue)
{
	GetMovementComponent()->AddInputVector(FVector(1.0f, 0.0f, 0.0f) * AxisValue);
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
	//Respawning logic
	if (bNewLifeStatus)
	{
		EnablePlayer();
		//Cancel the respawn timer in case we respawned from something else (game restarting, etc.).
		if (HasAuthority())
		{
			if (GetWorldTimerManager().IsTimerActive(RespawnHandle))
			{
				GetWorldTimerManager().ClearTimer(RespawnHandle);
			}
		}
	}
	//Dying logic
	else
	{
		DisablePlayer();	
		//Subtract lives, start respawn timer or end the game.
		if (HasAuthority())
		{
			CurrentLives--;
			OnRep_CurrentLives();
			if (CurrentLives <= 0)
			{
				AMarioGameState* GameState = Cast<AMarioGameState>(GetWorld()->GetGameState());
				if (IsValid(GameState))
				{
					GameState->PlayerExhaustedLives();
				}
			}
			else
			{
				GetWorldTimerManager().SetTimer(RespawnHandle, this, &AMarioPlayerCharacter::Respawn, RespawnDelay);
			}
		}
	}
}

void AMarioPlayerCharacter::Respawn()
{
	TeleportTo(CachedSpawnLocation, CachedSpawnRotation);
	HealthComponent->ResetHealth();
}

void AMarioPlayerCharacter::OnRep_CurrentLives()
{
	OnLivesChanged.Broadcast(CurrentLives);
}

void AMarioPlayerCharacter::SubscribeToLivesChanged(const FLivesCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLivesChanged.AddUnique(Callback);
	}
}

void AMarioPlayerCharacter::UnsubscribeFromLivesChanged(const FLivesCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLivesChanged.Remove(Callback);
	}
}

#pragma endregion
#pragma region Collision

void AMarioPlayerCharacter::OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue)
{
	if (IsLocallyControlled() && IsValid(MarioMoveComponent) && !BounceImpulse.IsNearlyZero())
	{
		MarioMoveComponent->TriggerBounce(PlayerHitbox->GetHitboxID(), CollidingHitbox->GetHitboxID());
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