#include "HealthComponent.h"
#include "UI/FloatingHealthBar.h"
#include "MarioGameState.h"
#include "MarioPlayerCharacter.h"
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

	//InitializeComponent also runs in the editor when loading the owning actor.
	//Check here that we are actually in the game.
	if (!IsValid(GetWorld()) || !GetWorld()->IsGameWorld())
	{
		return;
	}
	
	//Clamp max health above 1 before initializing current health.
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bShowHealthBar && IsValid(HealthBarWidgetClass))
	{
		//We only initialize health bars if the local player is valid. No need to do this on a dedicated server, for instance.
		GameStateRef = Cast<AMarioGameState>(GetWorld()->GetGameState());
		if (IsValid(GameStateRef))
		{
			//If the local player already exists, we can go ahead and set up our health bar widget,
			if (IsValid(GameStateRef->GetLocalMarioPlayer()))
			{
				InitHealthBarWidget();
			}
			//If the local player is not yet set up, we instead wait for a callback from the GameState.
			else
			{
				LocalPlayerInitCallback.BindDynamic(this, &UHealthComponent::OnLocalPlayerInit);
				GameStateRef->SubscribeToLocalPlayerInitialized(LocalPlayerInitCallback);
			}
		}
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

void UHealthComponent::OnLocalPlayerInit(AMarioPlayerCharacter* LocalPlayer)
{
	if (IsValid(LocalPlayer))
	{
		GameStateRef->UnsubscribeFromLocalPlayerInitialized(LocalPlayerInitCallback);
		InitHealthBarWidget();
	}
}

#pragma endregion 