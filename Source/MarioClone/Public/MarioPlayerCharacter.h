#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "HealthComponent.h"
#include "GameFramework/Character.h"
#include "MarioPlayerCharacter.generated.h"

class UHealthComponent;
class UCameraComponent;

UCLASS()
class MARIOCLONE_API AMarioPlayerCharacter : public ACharacter, public ICombatInterface
{
	GENERATED_BODY()

#pragma region Core

public:
	
	AMarioPlayerCharacter();
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxCameraLeadOffset = 300.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraLeadInterpSpeed = 300.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraRecenterDelay = 0.5f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraRecenterSpeed = 100.0f;

	FVector DesiredBaseCameraOffset;
	float TimeAtRest = 0.0f;

#pragma endregion 
#pragma region Movement

private:

	UFUNCTION()
	void MovementInput(const float AxisValue);
	UFUNCTION()
	void JumpPressed();
	UFUNCTION()
	void JumpReleased();

#pragma endregion
#pragma region Health

public:

	virtual UHealthComponent* GetHealthComponent() const override { return HealthComponent; }

private:

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	bool bRegenerateHealth = true;
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (EditCondition = "bRegenerateHealth", ClampMin = "0"))
	float InitialHealthRegenDelay = 5.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (EditCondition = "bRegenerateHealth", ClampMin = "0"))
	float HealthRegenerationTickRate = 1.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (EditCondition = "bRegenerateHealth", ClampMin = "0"))
	float HealthRegenerationTickSize = 1.0f;

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
	
#if WITH_EDITOR
	UFUNCTION()
	void DEBUG_DamageSelf();
	UFUNCTION(Server, Reliable)
	void Server_DEBUG_DamageSelf();
	UFUNCTION()
	void DEBUG_HealSelf();
	UFUNCTION(Server, Reliable)
	void Server_DEBUG_HealSelf();
	UFUNCTION()
	void DEBUG_KillSelf();
	UFUNCTION(Server, Reliable)
	void Server_DEBUG_KillSelf();
	UFUNCTION()
	void DEBUG_ReviveSelf();
	UFUNCTION(Server, Reliable)
	void Server_DEBUG_ReviveSelf();
#endif
	
#pragma endregion 
};