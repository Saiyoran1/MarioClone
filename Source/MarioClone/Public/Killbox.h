#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Killbox.generated.h"

class UBoxComponent;

UCLASS()
class MARIOCLONE_API AKillbox : public AActor
{
	GENERATED_BODY()

public:
	
	AKillbox();
	virtual void BeginPlay() override;

private:

	static const FName KillBoxProfile;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* CollisionBox;

	UFUNCTION()
	void OnCollision(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};