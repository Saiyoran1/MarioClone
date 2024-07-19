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
	UFUNCTION()
	void UpdateHealth(const float PreviousHealth, const float CurrentHealth, const float MaxHealth);
	FHealthCallback HealthCallback;
	UFUNCTION()
	void UpdateLife(const bool bNewLifeStatus);
	FLifeCallback LifeCallback;

	void UpdateHealthVisuals();
};