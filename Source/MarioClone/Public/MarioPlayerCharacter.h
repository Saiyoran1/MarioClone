﻿#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperCharacter.h"
#include "PaperFlipbook.h"
#include "UI/GameOverScreen.h"
#include "MarioPlayerCharacter.generated.h"

class UPlayerHUD;
class UMarioMovementComponent;
class UHealthComponent;
class UCameraComponent;

DECLARE_DYNAMIC_DELEGATE_OneParam(FLivesCallback, const int32, NewLives);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLivesNotification, const int32, NewLives);
DECLARE_DYNAMIC_DELEGATE_OneParam(FScoreCallback, const int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScoreNotification, const int32, NewScore);

UCLASS()
class MARIOCLONE_API AMarioPlayerCharacter : public APaperCharacter, public ICombatInterface
{
	GENERATED_BODY()

#pragma region Core

public:
	
	AMarioPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:

	UPROPERTY()
	AMarioGameState* GameStateRef = nullptr;
	FDelegateHandle GameStateDelegateHandle;
	UFUNCTION()
	void OnGameStateSet(AGameStateBase* GameState);

	void EnablePlayer();
	void DisablePlayer();
	bool bIsEnabled = true;

	UPROPERTY()
	APlayerController* PlayerController = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPlayerHUD> HUDClass;
	UPROPERTY()
	UPlayerHUD* HUD;

#pragma endregion
#pragma region Game Flow

private:

	FGameStartCallback GameStartCallback;
	UFUNCTION()
	void OnGameStarted();
	
	FGameEndCallback GameEndCallback;
	UFUNCTION()
	void OnGameEnded(const bool bGameWon);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGameOverScreen> GameOverScreenClass;
	UPROPERTY()
	UGameOverScreen* GameOverScreen = nullptr;
	
	FRestartCallback RestartCallback;
	UFUNCTION()
	void RestartRequested() { Server_RequestRestart(); }
	UFUNCTION(Server, Reliable)
	void Server_RequestRestart();

#pragma endregion 
#pragma region Animations

private:

	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UPaperFlipbook* IdleFlipbook = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UPaperFlipbook* RunningFlipbook = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	UPaperFlipbook* JumpingFlipbook = nullptr;

	bool bWasMovingRight = true;

#pragma endregion 
#pragma region Camera

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxCameraLeadOffset = 100.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraLeadInterpSpeed = 600.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraRecenterDelay = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraRecenterSpeed = 150.0f;

	FVector DesiredBaseCameraOffset;
	float TimeAtRest = 0.0f;

#pragma endregion 
#pragma region Movement

private:

	UPROPERTY()
	UMarioMovementComponent* MarioMoveComponent;

	UFUNCTION()
	void MovementInput(const float AxisValue);
	UFUNCTION()
	void JumpPressed();

#pragma endregion
#pragma region Health

public:

	virtual UHealthComponent* GetHealthComponent_Implementation() const override { return HealthComponent; }
	virtual void InstantKill_Implementation() override;
	
	int32 GetMaxLives() const { return MaxLives; }
	int32 GetCurrentLives() const { return CurrentLives; }

	void SubscribeToLivesChanged(const FLivesCallback& Callback);
	void UnsubscribeFromLivesChanged(const FLivesCallback& Callback);

private:

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	int32 MaxLives = 3;
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float RespawnDelay = 2.0f;

	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent = nullptr;
	
	FHealthCallback HealthCallback;
	UFUNCTION()
	void OnHealthChanged(const float PreviousHealth, const float CurrentHealth, const float MaxHealth);
	FLifeCallback LifeCallback;
	UFUNCTION()
	void OnLifeStatusChanged(const bool bNewLifeStatus);
	
	bool bRegeneratingHealth = false;
	FTimerHandle HealthRegenHandle;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentLives)
	int32 CurrentLives = 0;
	UFUNCTION()
	void OnRep_CurrentLives();
	FTimerHandle RespawnHandle;
	UFUNCTION()
	void Respawn();
	FVector CachedSpawnLocation;
	FRotator CachedSpawnRotation;
	FLivesNotification OnLivesChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float PostHitImmunityWindow = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	UMaterialInstance* SpriteFlashMaterial = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_bImmune)
	bool bImmune = false;
	UFUNCTION()
	void OnRep_bImmune();
	FTimerHandle ImmunityHandle;
	UFUNCTION()
	void EndImmunity();
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicSpriteFlashMaterial = nullptr;
	static constexpr int NumSpriteFlashCycles = 3;
	float SpriteFlashOpacity = 0.0f;
	float TimeSinceImmunityStart = 0.0f;
	UPROPERTY()
	UMaterialInterface* CachedSpriteMaterial = nullptr;
	
#pragma endregion
#pragma region Collision

public:

	virtual EHostility GetHostility_Implementation() const override { return EHostility::Friendly; }

private:
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHitbox* PlayerHitbox;
	FHitboxCallback HitboxCallback;
	UFUNCTION()
	void OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceToThis, const float DamageToThis, const FVector& BounceToOther, const float DamageToOther);

#pragma endregion
#pragma region Collectibles

public:

	int32 GetScore() const { return CollectibleScore; }
	//Called when colliding with a collectible on the server. Increases player score.
	void GrantCollectible(const int32 CollectibleValue);

	void SubscribeToScoreChanged(const FScoreCallback& Callback);
	void UnsubscribeFromScoreChanged(const FScoreCallback& Callback);

private:

	UPROPERTY(ReplicatedUsing = OnRep_CollectibleScore)
	int32 CollectibleScore = 0;
	UFUNCTION()
	void OnRep_CollectibleScore() { OnScoreChanged.Broadcast(CollectibleScore); }
	FScoreNotification OnScoreChanged;

#pragma endregion 
};