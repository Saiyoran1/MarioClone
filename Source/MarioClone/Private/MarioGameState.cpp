#include "MarioClone/Public/MarioGameState.h"
#include "MarioPlayerCharacter.h"
#include "GameFramework/GameMode.h"

FName AMarioGameState::PlayState = FName(TEXT("Playing"));
FName AMarioGameState::LossState = FName(TEXT("LostGame"));
FName AMarioGameState::WinState = FName(TEXT("WonGame"));

#pragma region Match State

void AMarioGameState::GoalReached()
{
	if (!HasAuthority())
	{
		return;
	}
	SetMatchState(WinState);
}

void AMarioGameState::PlayerExhaustedLives()
{
	if (!HasAuthority())
	{
		return;
	}
	SetMatchState(LossState);
}

void AMarioGameState::RequestRestartGame()
{
	if (!HasAuthority())
	{
		return;
	}
	SetMatchState(PlayState);
}

void AMarioGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();
	
	if (MatchState == PlayState)
	{
		OnGameStarted.Broadcast();
	}
	else if (MatchState == LossState)
	{
		OnGameEnded.Broadcast(false);
	}
	else if (MatchState == WinState)
	{
		OnGameEnded.Broadcast(true);
	}
	//If we just started the match for the first time, immediately go to our custom match state.
	else if (HasAuthority() && MatchState == MatchState::InProgress)
	{
		SetMatchState(PlayState);
	}
}

void AMarioGameState::SubscribeToGameStarted(const FGameStartCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnGameStarted.AddUnique(Callback);
	}
}

void AMarioGameState::UnsubscribeFromGameStarted(const FGameStartCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnGameStarted.Remove(Callback);
	}
}

void AMarioGameState::SubscribeToGameEnded(const FGameEndCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnGameEnded.AddUnique(Callback);
	}
}

void AMarioGameState::UnsubscribeFromGameEnded(const FGameEndCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnGameEnded.Remove(Callback);
	}
}

#pragma endregion 
#pragma region Player Init

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

#pragma endregion 