#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MarioGameState.generated.h"

class AMarioPlayerCharacter;

DECLARE_DYNAMIC_DELEGATE_OneParam(FPlayerInitializationCallback, AMarioPlayerCharacter*, LocalPlayerCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerInitializedNotification, AMarioPlayerCharacter*, LocalPlayerCharacter);

UCLASS()
class MARIOCLONE_API AMarioGameState : public AGameState
{
	GENERATED_BODY()

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
};
