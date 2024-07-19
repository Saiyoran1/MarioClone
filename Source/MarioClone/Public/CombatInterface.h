#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

class UHealthComponent;

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

	virtual UHealthComponent* GetHealthComponent() const;
};