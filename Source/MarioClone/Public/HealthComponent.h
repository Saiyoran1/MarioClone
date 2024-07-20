#pragma once
#include "CoreMinimal.h"
#include "MarioGameState.h"
#include "Components/WidgetComponent.h"
#include "HealthComponent.generated.h"

class UFloatingHealthBar;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FHealthCallback, const float, PreviousHealth, const float, CurrentHealth, const float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthNotification, const float, PreviousHealth, const float, CurrentHealth, const float, MaxHealth);
DECLARE_DYNAMIC_DELEGATE_OneParam(FLifeCallback, const bool, bNewLifeStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLifeNotification, const bool, bNewLifeStatus);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MARIOCLONE_API UHealthComponent : public UWidgetComponent
{
	GENERATED_BODY()

#pragma region Core

public:
	
	UHealthComponent();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	//Ref saved to bind/unbind a callback for local player initialization for delayed health bar widget setup.
	UPROPERTY()
	AMarioGameState* GameStateRef = nullptr;

#pragma endregion 
#pragma region Health
	
public:

	bool IsAlive() const { return bIsAlive; }
	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercentage() const { return MaxHealth == 0.0f ? 0.0f : FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f); }

	//Deals damage or healing to this component. Server-only.
	void ModifyHealth(const float HealthChange);
	//Instantly deals all remaining health as damage.
	void InstantKill();
	//Sets health back to full and changes status back to alive.
	void ResetHealth();
	
	void SubscribeToHealthChanged(const FHealthCallback& Callback);
	void UnsubscribeFromHealthChanged(const FHealthCallback& Callback);

	void SubscribeToLifeStatusChanged(const FLifeCallback& Callback);
	void UnsubscribeFromLifeStatusChanged(const FLifeCallback& Callback);

private:

	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 1.0f;
	
	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float CurrentHealth = 1.0f;
	UFUNCTION()
	void OnRep_Health(const float PreviousHealth);
	FHealthNotification OnHealthChanged;

	UPROPERTY(ReplicatedUsing = OnRep_IsAlive)
	bool bIsAlive = true;
	UFUNCTION()
	void OnRep_IsAlive();
	FLifeNotification OnLifeStatusChanged;

#pragma endregion
#pragma region Health Widget

private:

	//Whether to show a floating health bar widget above the owning actor.
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bShowHealthBar = true;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (EditCondition = "bShowHealthBar"))
	TSubclassOf<UFloatingHealthBar> HealthBarWidgetClass;

	UPROPERTY()
	UFloatingHealthBar* HealthBarWidget;
	void InitHealthBarWidget();

	//If the local player was not yet initialized during this component's setup, this callback will be called by the GameState after initialization.
	//This prevents setting up unnecessary widgets where no local player exists (dedicated servers).
	FPlayerInitializationCallback LocalPlayerInitCallback;
	UFUNCTION()
	void OnLocalPlayerInit(AMarioPlayerCharacter* LocalPlayer);

#pragma endregion 
};