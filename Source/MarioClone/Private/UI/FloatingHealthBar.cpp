#include "UI/FloatingHealthBar.h"
#include "CombatInterface.h"
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
	bInitialized = true;
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
	//Set the color of the health bar based on owner hostility. This only runs the first time we update the visuals (during init).
	if (!bInitialized && HealthComponent->GetOwner()->Implements<UCombatInterface>())
	{
		const EHostility OwnerHostility = ICombatInterface::Execute_GetHostility(HealthComponent->GetOwner());
		const FLinearColor HealthBarColor = OwnerHostility == EHostility::Enemy ? FLinearColor::Red
			: OwnerHostility == EHostility::Friendly ? FLinearColor::Green
			: /*OwnerHostility == EHostility::Neutral ? */ FLinearColor::Yellow;
		HealthBar->SetFillColorAndOpacity(HealthBarColor);
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