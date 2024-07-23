#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MarioGameState.generated.h"

class AMarioPlayerCharacter;

DECLARE_DYNAMIC_DELEGATE_OneParam(FPlayerInitializationCallback, AMarioPlayerCharacter*, LocalPlayerCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInitializedNotification, AMarioPlayerCharacter*, LocalPlayerCharacter);
DECLARE_DYNAMIC_DELEGATE(FGameStartCallback);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameStartNotification);
DECLARE_DYNAMIC_DELEGATE_OneParam(FGameEndCallback, const bool, bWonGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameEndNotification, const bool, bWonGame);

UCLASS()
class MARIOCLONE_API AMarioGameState : public AGameState
{
	GENERATED_BODY()

#pragma region Player Init

public:

	AMarioPlayerCharacter* GetLocalMarioPlayer() const { return LocalMarioPlayer; }
	void SubscribeToLocalPlayerInitialized(const FPlayerInitializationCallback& Callback);
	void UnsubscribeFromLocalPlayerInitialized(const FPlayerInitializationCallback& Callback);

	//Called when the local player is set up, so that we can initialize things that depend on it having a valid controller.
	void InitializePlayer(AMarioPlayerCharacter* NewPlayer);

private:
	
	UPROPERTY()
	AMarioPlayerCharacter* LocalMarioPlayer = nullptr;
	FPlayerInitializedNotification OnLocalPlayerInit;

#pragma endregion 
#pragma region Match State

public:
	
	//Called from the win or loss screen to reset the game.
	void RequestRestartGame();
	//Called when a player runs out of lives, to end the game in a loss.
	void PlayerExhaustedLives();

	void SubscribeToGameStarted(const FGameStartCallback& Callback);
	void UnsubscribeFromGameStarted(const FGameStartCallback& Callback);
	void SubscribeToGameEnded(const FGameEndCallback& Callback);
	void UnsubscribeFromGameEnded(const FGameEndCallback& Callback);

protected:
	
	virtual void OnRep_MatchState() override;
	
private:

	static FName PlayState;
	static FName LossState;
	static FName WinState;
	FGameStartNotification OnGameStarted;
	FGameEndNotification OnGameEnded;
};
