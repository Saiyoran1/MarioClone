﻿#include "HealthComponent.h"

#include "FloatingHealthBar.h"
#include "Net/UnrealNetwork.h"

#pragma region Core

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UHealthComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
	//Clamp max health above 1 before initializing current health.
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bShowHealthBar && IsValid(HealthBarWidgetClass))
	{
		InitHealthBarWidget();
	}
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
	DOREPLIFETIME(UHealthComponent, bIsAlive);
}

#pragma endregion 
#pragma region Health

void UHealthComponent::ModifyHealth(const float HealthChange)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	if (!bIsAlive)
	{
		return;
	}
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealthChange, 0.0f, MaxHealth);
	if (CurrentHealth != PreviousHealth)
	{
		OnRep_Health(PreviousHealth);
		if (CurrentHealth == 0.0f)
		{
			bIsAlive = false;
			OnRep_IsAlive();
		}
	}
}

void UHealthComponent::InstantKill()
{
	ModifyHealth(MaxHealth * -1.0f);
}

void UHealthComponent::ResetHealth()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}
	const bool bWasAlive = bIsAlive;
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = MaxHealth;
	bIsAlive = true;
	if (PreviousHealth != CurrentHealth)
	{
		OnRep_Health(PreviousHealth);
	}
	if (bWasAlive != bIsAlive)
	{
		OnRep_IsAlive();
	}
}

void UHealthComponent::OnRep_Health(const float PreviousHealth)
{
	OnHealthChanged.Broadcast(PreviousHealth, CurrentHealth, MaxHealth);
}

void UHealthComponent::OnRep_IsAlive()
{
	OnLifeStatusChanged.Broadcast(bIsAlive);
}

void UHealthComponent::SubscribeToHealthChanged(const FHealthCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnHealthChanged.AddUnique(Callback);
	}
}

void UHealthComponent::UnsubscribeFromHealthChanged(const FHealthCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnHealthChanged.Remove(Callback);
	}
}

void UHealthComponent::SubscribeToLifeStatusChanged(const FLifeCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLifeStatusChanged.AddUnique(Callback);
	}
}

void UHealthComponent::UnsubscribeFromLifeStatusChanged(const FLifeCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLifeStatusChanged.Remove(Callback);
	}
}

#pragma endregion
#pragma region Health Widget

void UHealthComponent::InitHealthBarWidget()
{
	SetWidgetClass(HealthBarWidgetClass);
	HealthBarWidget = Cast<UFloatingHealthBar>(GetWidget());
	if (IsValid(HealthBarWidget))
	{
		//Widget components must tick to draw their widget.
		SetComponentTickEnabled(true);
		SetWidgetSpace(EWidgetSpace::Screen);
		SetDrawAtDesiredSize(true);
		HealthBarWidget->Init(this);
	}
}

#pragma endregion 