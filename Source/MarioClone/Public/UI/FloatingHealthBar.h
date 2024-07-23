#pragma once
#include "CoreMinimal.h"
#include "HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "FloatingHealthBar.generated.h"

class UProgressBar;

UCLASS()
class MARIOCLONE_API UFloatingHealthBar : public UUserWidget
{
	GENERATED_BODY()

public:
	
	void Init(UHealthComponent* HealthComp);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* HealthBar = nullptr;

	UPROPERTY()
	UHealthComponent* HealthComponent;
	//Callback from owning component that is fired when health value changes.
	FHealthCallback HealthCallback;
	UFUNCTION()
	void UpdateHealth(const float PreviousHealth, const float CurrentHealth, const float MaxHealth);
	//Callback from owning component that is fired when the owner dies or respawns.
	FLifeCallback LifeCallback;
	UFUNCTION()
	void UpdateLife(const bool bNewLifeStatus);

	bool bInitialized = false;

	void UpdateHealthVisuals();
};