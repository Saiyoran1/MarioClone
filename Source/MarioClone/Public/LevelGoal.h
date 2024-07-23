#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelGoal.generated.h"

class USphereComponent;

UCLASS()
class MARIOCLONE_API ALevelGoal : public AActor
{
	GENERATED_BODY()

public:

	ALevelGoal();
	virtual void BeginPlay() override;

private:

	static FName GoalCollisionProfile;
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionBox;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};