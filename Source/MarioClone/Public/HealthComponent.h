#pragma once
#include "CoreMinimal.h"
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

#pragma endregion 
#pragma region Health
	
public:

	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercentage() const { return MaxHealth == 0.0f ? 0.0f : FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f); }
	bool IsAlive() const { return bIsAlive; }
	
	void ModifyHealth(const float HealthChange);
	void InstantKill();
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

	//Whether to show a floating health bar widget.
	UPROPERTY(EditAnywhere, Category = "Health")
	bool bShowHealthBar = true;
	UPROPERTY(EditAnywhere, Category = "Health", meta = (EditCondition = "bShowHealthBar"))
	TSubclassOf<UFloatingHealthBar> HealthBarWidgetClass;

	UPROPERTY()
	UFloatingHealthBar* HealthBarWidget;
	void InitHealthBarWidget();

#pragma endregion 
};