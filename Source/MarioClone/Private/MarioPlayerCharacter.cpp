#include "MarioClone/Public/MarioPlayerCharacter.h"
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
	if (IsValid(Camera))
	{
		Camera->SetupAttachment(RootComponent);
		Camera->SetUsingAbsoluteRotation(true);
		Camera->SetWorldRotation(FRotator(0.0f, -90.0f, 0.0f));
		Camera->SetRelativeLocation(FVector(0.0f, 800.0f, 100.0f));
	}
	
	if (IsValid(GetSprite()))
	{
		GetSprite()->SetUsingAbsoluteRotation(true);
		GetSprite()->SetWorldRotation(FRotator(0.0f));
	}
	
	PlayerHitbox = CreateDefaultSubobject<UHitbox>(FName(TEXT("Hitbox")));
	if (IsValid(PlayerHitbox))
	{
		PlayerHitbox->SetupAttachment(RootComponent);
	}

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	if (IsValid(HealthComponent))
	{
		HealthComponent->SetupAttachment(RootComponent);
	}

	MarioMoveComponent = Cast<UMarioMovementComponent>(GetCharacterMovement());
}

void AMarioPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMarioPlayerCharacter, CurrentLives, COND_OwnerOnly);
	DOREPLIFETIME(AMarioPlayerCharacter, bImmune);
	DOREPLIFETIME_CONDITION(AMarioPlayerCharacter, CollectibleScore, COND_OwnerOnly);
}

void AMarioPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("Movement"), this, &AMarioPlayerCharacter::MovementInput);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &AMarioPlayerCharacter::JumpPressed);
}

void AMarioPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (IsValid(Controller.Get()) && Controller->IsLocalPlayerController())
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

void AMarioPlayerCharacter::OnGameStateSet(AGameStateBase* GameState)
{
	GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
	if (IsValid(GameStateRef))
	{
		GetWorld()->GameStateSetEvent.Remove(GameStateDelegateHandle);
		GameStateRef->InitializePlayer(this);
	}
}

void AMarioPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentLives = MaxLives;
	OnLivesChanged.Broadcast(CurrentLives);
	RespawnDelay = FMath::Max(0.0f, RespawnDelay);
	CachedSpawnLocation = GetActorLocation();
	CachedSpawnRotation = GetActorRotation();
	CachedSpriteMaterial = IsValid(GetSprite()) ? GetSprite()->GetMaterial(0) : nullptr;
	
	//Constrain movement to the X and Z axes, since this is a 2D game.
	if (IsValid(GetMovementComponent()))
	{
		GetMovementComponent()->SetPlaneConstraintEnabled(true);
		GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);
	}

	//Bind to the health and life status delegates of the health component.
	if (IsValid(HealthComponent))
	{
		HealthCallback.BindDynamic(this, &AMarioPlayerCharacter::OnHealthChanged);
		LifeCallback.BindDynamic(this, &AMarioPlayerCharacter::OnLifeStatusChanged);
		HealthComponent->SubscribeToHealthChanged(HealthCallback);
		HealthComponent->SubscribeToLifeStatusChanged(LifeCallback);
	}

	if (IsValid(PlayerHitbox))
	{
		HitboxCallback.BindDynamic(this, &AMarioPlayerCharacter::OnHitboxCollision);
		PlayerHitbox->SubscribeToHitboxCollision(HitboxCallback);
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

	//If the player is disabled (dead or the game has ended), early exit.
	if (!bIsEnabled)
	{
		return;
	}
	//If the player is dead, we can early exit here and not try to adjust our sprite.
	if (IsValid(HealthComponent) && !HealthComponent->IsAlive())
	{
		return;
	}

	//Update our sprite flash material visual while immune.
	if (bImmune)
	{
		TimeSinceImmunityStart += DeltaSeconds;
		if (IsValid(DynamicSpriteFlashMaterial))
		{
			const float Frequency = NumSpriteFlashCycles / PostHitImmunityWindow;
			//Shamelessly stolen sine function from stackoverflow to make opacity pulse.
			const float Opacity = 0.5f * (1 + FMath::Sin(2 * PI * Frequency * TimeSinceImmunityStart));
			DynamicSpriteFlashMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
		}
	}

	if (!IsValid(GetSprite()))
	{
		return;
	}
	//Determine which sprite flipbook to use based on whether we are jumping, running, or standing still.
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

	//Fallback in case all flipbooks were null or our movement component was null.
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

void AMarioPlayerCharacter::EnablePlayer()
{
	if (bIsEnabled)
	{
		return;
	}
	
	SetActorTickEnabled(true);
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
	EndImmunity();

	bIsEnabled = true;
}

void AMarioPlayerCharacter::DisablePlayer()
{
	if (!bIsEnabled)
	{
		return;
	}
	
	SetActorTickEnabled(false);
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
	//If we were immune, clear the immunity timer.
	EndImmunity();

	bIsEnabled = false;
}

#pragma endregion
#pragma region Game Flow

void AMarioPlayerCharacter::OnGameStarted()
{
	if (HasAuthority())
	{
		CurrentLives = MaxLives;
		OnRep_CurrentLives();
		Respawn();
		
		CollectibleScore = 0;
		OnRep_CollectibleScore();
	}
	
	EnablePlayer();
	
	if (IsLocallyControlled())
	{
		if (IsValid(GameOverScreen))
		{
			GameOverScreen->RemoveFromParent();
			GameOverScreen = nullptr;
		}
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

#pragma endregion 
#pragma region Movement

void AMarioPlayerCharacter::MovementInput(const float AxisValue)
{
	if (IsValid(GetMovementComponent()))
	{
		GetMovementComponent()->AddInputVector(FVector(1.0f, 0.0f, 0.0f) * AxisValue);
	}
}

void AMarioPlayerCharacter::JumpPressed()
{
	Jump();
}

#pragma endregion 
#pragma region Health

void AMarioPlayerCharacter::OnHealthChanged(const float PreviousHealth, const float CurrentHealth, const float MaxHealth)
{
	//If we took damage, we should briefly become immune to damage.
	if (HasAuthority() && bIsEnabled
		&& CurrentHealth < PreviousHealth && CurrentHealth > 0.0f
		&& PostHitImmunityWindow > 0.0f)
	{
		bImmune = true;
		OnRep_bImmune();
		GetWorldTimerManager().SetTimer(ImmunityHandle, this, &AMarioPlayerCharacter::EndImmunity, PostHitImmunityWindow);
	}
}

void AMarioPlayerCharacter::EndImmunity()
{
	if (GetWorldTimerManager().IsTimerActive(ImmunityHandle))
	{
		GetWorldTimerManager().ClearTimer(ImmunityHandle);
	}
	bImmune = false;
	OnRep_bImmune();
}

void AMarioPlayerCharacter::OnRep_bImmune()
{
	if (bIsEnabled)
	{
		if (bImmune)
		{
			if (IsValid(GetSprite()) && IsValid(SpriteFlashMaterial))
			{
				DynamicSpriteFlashMaterial = UMaterialInstanceDynamic::Create(SpriteFlashMaterial, this);
				if (IsValid(DynamicSpriteFlashMaterial))
				{
					GetSprite()->SetMaterial(0, DynamicSpriteFlashMaterial);
				}
			}
			TimeSinceImmunityStart = 0.0f;
		}
		else
		{
			if (IsValid(GetSprite()) && IsValid(CachedSpriteMaterial))
			{
				GetSprite()->SetMaterial(0, CachedSpriteMaterial);
				DynamicSpriteFlashMaterial = nullptr;
			}
			TimeSinceImmunityStart = 0.0f;
		}
	}
}

void AMarioPlayerCharacter::OnLifeStatusChanged(const bool bNewLifeStatus)
{
	//Respawning logic
	if (bNewLifeStatus)
	{
		//Cancel the respawn timer in case we respawned from something else (game restarting, etc.).
		if (HasAuthority())
		{
			if (GetWorldTimerManager().IsTimerActive(RespawnHandle))
			{
				GetWorldTimerManager().ClearTimer(RespawnHandle);
			}
		}
		EnablePlayer();
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

void AMarioPlayerCharacter::InstantKill_Implementation()
{
	if (bImmune)
	{
		EndImmunity();
	}
	if (IsValid(HealthComponent))
	{
		HealthComponent->InstantKill();
	}
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

void AMarioPlayerCharacter::OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceToThis, const float DamageToThis, const FVector& BounceToOther, const float DamageToOther)
{
	if (IsLocallyControlled() && IsValid(MarioMoveComponent))
	{
		MarioMoveComponent->OnHitboxCollision(PlayerHitbox->GetHitboxID(), CollidingHitbox->GetHitboxID(),
			BounceToThis != FVector::ZeroVector, BounceToOther != FVector::ZeroVector, DamageToThis != 0.0f, DamageToOther != 0.0f);
	}
	if (HasAuthority() && !bImmune && DamageToThis != 0.0f && IsValid(HealthComponent))
	{
		HealthComponent->ModifyHealth(-1.0f * DamageToThis);
	}
}

#pragma endregion
#pragma region Collectibles

void AMarioPlayerCharacter::GrantCollectible(const int32 CollectibleValue)
{
	if (HasAuthority())
	{
		CollectibleScore += CollectibleValue;
		OnRep_CollectibleScore();
	}
}

void AMarioPlayerCharacter::SubscribeToScoreChanged(const FScoreCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnScoreChanged.AddUnique(Callback);
	}
}

void AMarioPlayerCharacter::UnsubscribeFromScoreChanged(const FScoreCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnScoreChanged.Remove(Callback);
	}
}

#pragma endregion 