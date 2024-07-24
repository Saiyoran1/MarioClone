#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class UHealthComponent;

UENUM()
enum class EHostility : uint8
{
	Neutral = 0,
	Enemy = 1,
	Friendly = 2
};

//Basic interface for getting components related to combat from an actor in the world.
UINTERFACE()
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class MARIOCLONE_API ICombatInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	UHealthComponent* GetHealthComponent() const;
	virtual UHealthComponent* GetHealthComponent_Implementation() const;
	
	UFUNCTION(BlueprintNativeEvent)
	EHostility GetHostility() const;
	virtual EHostility GetHostility_Implementation() const { return EHostility::Enemy; }

	UFUNCTION(BlueprintNativeEvent)
	void InstantKill();
};