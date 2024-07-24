#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RespawnIndicator.generated.h"

class UWidgetComponent;
class URespawnWidget;
class ANPCCharacter;

UCLASS()
class MARIOCLONE_API ARespawnIndicator : public AActor
{
	GENERATED_BODY()

public:
	
	ARespawnIndicator();

	void Init(ANPCCharacter* RespawningCharacter);

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess))
	UWidgetComponent* WidgetComponent;
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	TSubclassOf<URespawnWidget> WidgetClass;
};