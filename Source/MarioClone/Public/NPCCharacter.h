#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperCharacter.h"
#include "NPCCharacter.generated.h"

class ARespawnIndicator;

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

UCLASS()
class MARIOCLONE_API ANPCCharacter : public APaperCharacter, public ICombatInterface
{
	GENERATED_BODY()

public:
	
	ANPCCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EHostility GetHostility_Implementation() const override { return Hostility; }
	virtual UHealthComponent* GetHealthComponent_Implementation() const override { return HealthComponent; }
	virtual void InstantKill_Implementation() override;

	float GetRespawnTime() const { return RespawnInfo.bRespawning ? RespawnInfo.RespawnTime : -1.0f; }

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
	
	void DisableNPC();
	void EnableNPC();
	bool bIsEnabled = true;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHitbox* Hitbox = nullptr;
	FHitboxCallback HitboxCallback;
	UFUNCTION()
	void OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue);
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent = nullptr;
	FLifeCallback LifeCallback;
	UFUNCTION()
	void OnLifeStatusChanged(const bool bNewLifeStatus);

	UPROPERTY(EditAnywhere, Category = "Hostility")
	EHostility Hostility = EHostility::Enemy;

	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bShouldMove = true;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BumperDistanceHorizontal = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BumperDistanceVertical = 100.0f;
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove"))
	bool bShouldRandomlyJump = true;
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump"))
	float RandomJumpInterval = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump"))
	float RandomJumpIntervalVariance = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (EditCondition = "bShouldMove && bShouldRandomlyJump"))
	float RandomJumpChance = 0.3f;

	static const FName BumperProfile;
	bool bWasMovingForward = true;
	float TimeTilJumpAttempt = 0.0f;

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
	UFUNCTION()
	void OnRespawnTimer();
	void CancelRespawn();
	UPROPERTY()
	ARespawnIndicator* RespawnIndicator;
};