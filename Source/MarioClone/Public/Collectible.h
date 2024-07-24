// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarioGameState.h"
#include "GameFramework/Actor.h"
#include "Collectible.generated.h"

class UPaperSpriteComponent;
class USphereComponent;

UCLASS()
class MARIOCLONE_API ACollectible : public AActor
{
	GENERATED_BODY()

public:
	
	ACollectible();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

private:

	static const FName CollectibleProfile;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	USphereComponent* CollisionSphere;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	UPaperSpriteComponent* Sprite;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	//How much this collectible is worth to a player.
	UPROPERTY(EditAnywhere, Category = "Collectible")
	int32 CollectibleValue = 1;

	UPROPERTY(ReplicatedUsing = OnRep_bCollected)
	bool bCollected = false;
	UFUNCTION()
	void OnRep_bCollected();

	UPROPERTY()
	AMarioGameState* GameStateRef = nullptr;
	FDelegateHandle GameStateDelegateHandle;
	UFUNCTION()
	void OnGameStateSet(AGameStateBase* GameState);
	FGameStartCallback GameStartCallback;
	UFUNCTION()
	void OnGameStarted();
};