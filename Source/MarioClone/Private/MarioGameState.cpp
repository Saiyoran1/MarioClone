#include "MarioClone/Public/MarioGameState.h"
#include "MarioPlayerCharacter.h"

void AMarioGameState::InitializePlayer(AMarioPlayerCharacter* NewPlayer)
{
	if (!IsValid(NewPlayer) || !NewPlayer->IsLocallyControlled()
		|| LocalMarioPlayer == NewPlayer)
	{
		return;
	}
	LocalMarioPlayer = NewPlayer;
	OnLocalPlayerInit.Broadcast(NewPlayer);
}

void AMarioGameState::SubscribeToLocalPlayerInitialized(const FPlayerInitializationCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLocalPlayerInit.AddUnique(Callback);
	}
}

void AMarioGameState::UnsubscribeFromLocalPlayerInitialized(const FPlayerInitializationCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnLocalPlayerInit.Remove(Callback);
	}
}