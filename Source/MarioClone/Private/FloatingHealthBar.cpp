#include "FloatingHealthBar.h"
#include "HealthComponent.h"
#include "Components/ProgressBar.h"

void UFloatingHealthBar::Init(UHealthComponent* HealthComp)
{
	if (!IsValid(HealthComp))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	HealthComponent = HealthComp;
	HealthCallback.BindDynamic(this, &UFloatingHealthBar::UpdateHealth);
	LifeCallback.BindDynamic(this, &UFloatingHealthBar::UpdateLife);
	HealthComponent->SubscribeToHealthChanged(HealthCallback);
	HealthComponent->SubscribeToLifeStatusChanged(LifeCallback);
	UpdateHealthVisuals();
}

void UFloatingHealthBar::UpdateHealth(const float PreviousHealth, const float CurrentHealth, const float MaxHealth)
{
	UpdateHealthVisuals();
}

void UFloatingHealthBar::UpdateLife(const bool bNewLifeStatus)
{
	UpdateHealthVisuals();
}

void UFloatingHealthBar::UpdateHealthVisuals()
{
	if (!IsValid(HealthBar) || !IsValid(HealthComponent))
	{
		return;
	}
	const float HealthPercentage = HealthComponent->GetHealthPercentage();
	//We will show the health bar only if the associated component is both alive and not at full health.
	if (HealthComponent->IsAlive() && HealthPercentage > 0.0f && HealthPercentage < 1.0f)
	{
		if (GetVisibility() == ESlateVisibility::Collapsed)
		{
			SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		HealthBar->SetPercent(HealthPercentage);
	}
	else
	{
		if (GetVisibility() != ESlateVisibility::Collapsed)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}