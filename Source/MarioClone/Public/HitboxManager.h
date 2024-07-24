#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HitboxManager.generated.h"

class UHitbox;

USTRUCT()
struct FHitboxSnapshot
{
	GENERATED_BODY()

	float Timestamp = 0.0f;
	FVector Location = FVector::ZeroVector;

	FHitboxSnapshot() {}
	FHitboxSnapshot(const float InTime, const FVector& InLoc) : Timestamp(InTime), Location(InLoc) {}
};

USTRUCT()
struct FHitboxSnapshotArray
{
	GENERATED_BODY();

	TArray<FHitboxSnapshot> Snapshots;
};

UCLASS()
class MARIOCLONE_API UHitboxManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override { return TStatId(); }
	virtual void Tick(float DeltaTime) override;
	
	FVector GetBounceImpulseForHitbox(const int32 HitboxID) const;
	bool SanityCheckBounce(const int32 HitboxIDA, const int32 HitboxIDB, const float PingTime) const;
	int32 RegisterNewHitbox(UHitbox* Hitbox);
	void RegisterNewHitbox(UHitbox* Hitbox, const int32 ID);

	void ConfirmCollisionOfHitboxes(const int32 InstigatorID, const int32 TargetID, const bool bDamage, const bool bBounce);
	
private:

	int32 HitboxIDCounter = 0;
	UPROPERTY()
	TMap<int32, UHitbox*> HitboxMap;
	
	static constexpr int MaxSnapshots = 50;
	static constexpr float HitboxToleranceMultiplier = 2.0f;
	UPROPERTY()
	TMap<int32, FHitboxSnapshotArray> HitboxSnapshots;
	FVector GetHitboxPositionAtTime(const int32 HitboxID, const float Timestamp) const;
	
};
