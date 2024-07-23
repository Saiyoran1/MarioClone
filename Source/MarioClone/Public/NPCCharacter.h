#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperCharacter.h"
#include "NPCCharacter.generated.h"

UCLASS()
class MARIOCLONE_API ANPCCharacter : public APaperCharacter, public ICombatInterface
{
	GENERATED_BODY()

public:
	
	ANPCCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual EHostility GetHostility_Implementation() const override { return Hostility; }
	virtual UHealthComponent* GetHealthComponent_Implementation() const override { return HealthComponent; } 

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
	void OnGameEnd(const bool bGameWon) { DisableNPC(); }
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
};