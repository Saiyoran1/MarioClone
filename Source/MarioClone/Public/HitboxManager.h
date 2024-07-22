#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitboxManager.generated.h"

class UHitbox;

UCLASS()
class MARIOCLONE_API UHitboxManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	FVector GetBounceImpulseForHitbox(const int32 HitboxID) const;
	bool SanityCheckBounce(const int32 HitboxIDA, const int32 HitboxIDB) const;
	int32 RegisterNewHitbox(const UHitbox* Hitbox);
	void RegisterNewHitbox(const UHitbox* Hitbox, const int32 ID);

private:

	UPROPERTY()
	TMap<int32, const UHitbox*> HitboxMap;

	int32 HitboxIDCounter = 0;
};
