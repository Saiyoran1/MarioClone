#include "CombatInterface.h"
#include "HealthComponent.h"

UHealthComponent* ICombatInterface::GetHealthComponent() const
{
	//If we got here, that means the actor implementing this interface didn't implement GetHealthComponent.
	//We can just call FindComponentByClass, but its slower than the actor just returning the pointer they already have cached.
	UE_LOG(LogTemp, Warning, TEXT("Called default implementation of ICombatInterface::GetHealthComponent. This iterates over owner components. Please implement the function."));
	if (IsValid(_getUObject()))
	{
		AActor* OwnerActor = Cast<AActor>(_getUObject());
		if (IsValid(OwnerActor))
		{
			return OwnerActor->FindComponentByClass<UHealthComponent>();
		}
	}
	return nullptr;
}