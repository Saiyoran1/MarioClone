#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperCharacter.h"
#include "NPCCharacter.generated.h"

class ARespawnIndicator;

//Struct for respawn information that can be replicated to let clients show respawn indicators.
USTRUCT()
struct FRespawnInfo
{
	GENERATED_BODY();

	UPROPERTY()
	bool bRespawning = false;
	UPROPERTY()
	float RespawnTime = -1.0f;
	UPROPERTY()
	FVector RespawnLocation = FVector::ZeroVector;
};

//Base class for enemy NPCs in the game with common behavior such as movement, jumping, and damage handling.
UCLASS()
class MARIOCLONE_API ANPCCharacter : public APaperCharacter, public ICombatInterface
{
	GENERATED_BODY()

#pragma region Core

public:
	
	ANPCCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY()
	AMarioGameState* GameStateRef = nullptr;
	FDelegateHandle GameStateDelegateHandle;
	UFUNCTION()
	void OnGameStateSet(AGameStateBase* GameState);
	void BindToGameStartEnd();
	FGameStartCallback GameStartCallback;
	UFUNCTION()
	void OnGameStart();
	FGameEndCallback GameEndCallback;
	UFUNCTION()
	void OnGameEnd(const bool bGameWon);

	FVector CachedStartLocation;
	FRotator CachedStartRotation;

	void EnableNPC();
	void DisableNPC();
	bool bIsEnabled = true;

#pragma endregion 
#pragma region Health

public:

	virtual EHostility GetHostility_Implementation() const override { return Hostility; }
	virtual UHealthComponent* GetHealthComponent_Implementation() const override { return HealthComponent; }
	virtual void InstantKill_Implementation() override;

	//When respawning, returns the timestamp at which the respawn will occur.
	float GetRespawnTime() const { return RespawnInfo.bRespawning ? RespawnInfo.RespawnTime : -1.0f; }

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent = nullptr;
	FLifeCallback LifeCallback;
	UFUNCTION()
	void OnLifeStatusChanged(const bool bNewLifeStatus);

	UPROPERTY(EditAnywhere, Category = "Hostility")
	EHostility Hostility = EHostility::Enemy;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHitbox* Hitbox = nullptr;
	FHitboxCallback HitboxCallback;
	UFUNCTION()
	void OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue);

	UPROPERTY(EditAnywhere, Category = "Health")
	bool bCanRespawn = false;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (EditCondition = "bCanRespawn"))
	float RespawnDelay = 10.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	TSubclassOf<ARespawnIndicator> RespawnIndicatorClass;

	UPROPERTY(ReplicatedUsing = OnRep_RespawnInfo)
	FRespawnInfo RespawnInfo;
	UFUNCTION()
	void OnRep_RespawnInfo(const FRespawnInfo& PreviousInfo);
	
	void StartRespawn();
	FTimerHandle RespawnHandle;
	//Called on the server to trigger the respawn.
	UFUNCTION()
	void OnRespawnTimer();
	void CancelRespawn();

	//Actor that exists in the world to warn players of this NPC's incoming respawn.
	UPROPERTY()
	ARespawnIndicator* RespawnIndicator;

#pragma endregion
#pragma region Movement

	static const FName BumperProfile;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bShouldMove = true;
	//How far in front of the NPC to check for walls to decide when to turn around.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BumperDistanceHorizontal = 50.0f;
	//How far down from the NPC's center to check for a floor in front of the NPC to decide to turn around.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BumperDistanceVertical = 100.0f;
	//Whether the NPC should occasionally try to jump to change up its movement.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove"))
	bool bShouldRandomlyJump = true;
	//How often the NPC tries to jump. Not every attempt will actually jump the NPC, it will just roll a random number against its RandomJumpChance.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump"))
	float RandomJumpInterval = 1.0f;
	//Adds some variance to the time between jump attempts.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump"))
	float RandomJumpIntervalVariance = 0.1f;
	//When attempting to jump, this number is how likely the NPC is to succeed in jumping.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump", ClampMin = "0", ClampMax = "1"))
	float RandomJumpChance = 0.3f;
	
	bool bWasMovingForward = true;
	float TimeTilJumpAttempt = 0.0f;

#pragma endregion 
};