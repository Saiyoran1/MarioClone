#include "HitboxManager.h"
#include "Hitbox.h"
#include "GameFramework/GameStateBase.h"

void UHitboxManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	HitboxMap.Empty();
}

void UHitboxManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//No need to tick on clients.
	if (GetWorld()->IsNetMode(NM_Client))
	{
		return;
	}

	for (const TTuple<int32, UHitbox*> HitboxTuple : HitboxMap)
	{
		const UHitbox* Hitbox = HitboxTuple.Value;
		if (IsValid(Hitbox))
		{
			if (FHitboxSnapshotArray* SnapshotArray = HitboxSnapshots.Find(HitboxTuple.Key))
			{
				SnapshotArray->Snapshots.Add(FHitboxSnapshot(GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), Hitbox->GetComponentLocation()));
				//Remove the oldest snapshots if we have more than our buffer.
				while (SnapshotArray->Snapshots.Num() > MaxSnapshots)
				{
					SnapshotArray->Snapshots.RemoveAt(0);
				}
			}
		}
	}
}

int32 UHitboxManager::RegisterNewHitbox(UHitbox* Hitbox)
{
	if (!IsValid(Hitbox))
	{
		return -1;
	}
	const int32 NewID = HitboxIDCounter++;
	HitboxMap.Add(NewID, Hitbox);
	//Create a snapshot array for this hitbox so the server can validate client collisions.
	FHitboxSnapshotArray& SnapshotArray = HitboxSnapshots.Add(NewID);
	//Add an initial snapshot.
	SnapshotArray.Snapshots.Add(FHitboxSnapshot(GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), Hitbox->GetComponentLocation()));
	return NewID;
}

void UHitboxManager::RegisterNewHitbox(UHitbox* Hitbox, const int32 ID)
{
	if (ID == -1 || !IsValid(Hitbox))
	{
		return;
	}
	HitboxMap.Add(ID, Hitbox);
	//This overload is called on clients, so we don't need to do anything to the snapshot map.
}

void UHitboxManager::ConfirmCollisionOfHitboxes(const int32 InstigatorID, const int32 TargetID, const bool bDamage, const bool bBounce)
{
	if (!bDamage && !bBounce)
	{
		return;
	}
	UHitbox* InstigatorHitbox = HitboxMap.FindRef(InstigatorID);
	if (!IsValid(InstigatorHitbox))
	{
		return;
	}
	UHitbox* TargetHitbox = HitboxMap.FindRef(TargetID);
	if (!IsValid(TargetHitbox))
	{
		return;
	}
	const float Damage = bDamage ? InstigatorHitbox->GetCollisionDamageDone() : 0.0f;
	const FVector Impulse = bBounce ? InstigatorHitbox->GetBounceImpulse() : FVector::ZeroVector;
	TargetHitbox->NotifyOfCollisionResult(InstigatorHitbox, Impulse, Damage, FVector::ZeroVector, 0.0f);
}

FVector UHitboxManager::GetHitboxPositionAtTime(const int32 HitboxID, const float Timestamp) const
{
	const UHitbox* Hitbox = HitboxMap.FindRef(HitboxID);
	if (!IsValid(Hitbox))
	{
		return FVector::ZeroVector;
	}
	const FHitboxSnapshotArray* SnapshotArray = HitboxSnapshots.Find(HitboxID);
	//If this hitbox doesn't have a snapshot array or doesn't have snapshots, just return the hitbox's current location.
	if (!SnapshotArray || SnapshotArray->Snapshots.Num() == 0)
	{
		return Hitbox->GetComponentLocation();
	}
	//Find a snapshot before the timestamp and a snapshot after the timestamp, and then lerp between them to guess at the position at the timestamp.
	float TimeAfter = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	FVector PositionAfter = Hitbox->GetComponentLocation();
	float TimeBefore = 0.0f;
	FVector PositionBefore = FVector::ZeroVector;
	bool bFoundPositionBefore = false;
	for (int i = SnapshotArray->Snapshots.Num() - 1; i >= 0; i--)
	{
		if (SnapshotArray->Snapshots[i].Timestamp >= Timestamp)
		{
			TimeAfter = SnapshotArray->Snapshots[i].Timestamp;
			PositionAfter = SnapshotArray->Snapshots[i].Location;
		}
		else
		{
			TimeBefore = SnapshotArray->Snapshots[i].Timestamp;
			PositionBefore = SnapshotArray->Snapshots[i].Location;
			bFoundPositionBefore = true;
			break;
		}
	}
	if (!bFoundPositionBefore)
	{
		return PositionAfter;
	}
	return FMath::Lerp(PositionBefore, PositionAfter, FMath::Clamp((Timestamp - TimeBefore) / (TimeAfter - TimeBefore), 0.0f, 1.0f));
}

FVector UHitboxManager::GetBounceImpulseForHitbox(const int32 HitboxID) const
{
	if (HitboxID == -1)
	{
		return FVector::ZeroVector;
	}
	const UHitbox* Hitbox = HitboxMap.FindRef(HitboxID);
	if (IsValid(Hitbox))
	{
		return Hitbox->GetBounceImpulse();
	}
	else
	{
		return FVector::ZeroVector;
	}
}

bool UHitboxManager::SanityCheckBounce(const int32 HitboxIDA, const int32 HitboxIDB, const float PingTime) const
{
	const UHitbox* HitboxA = HitboxMap.FindRef(HitboxIDA);
	if (!IsValid(HitboxA))
	{
		return false;
	}
	const UHitbox* HitboxB = HitboxMap.FindRef(HitboxIDB);
	if (!IsValid(HitboxB))
	{
		return false;
	}
	//If we have no game state reference, things have gone wrong.
	if (!IsValid(GetWorld()->GetGameState()))
	{
		return false;
	}
	const float CurrentTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const FVector PositionA = HitboxA->GetComponentLocation();
	const FVector PositionB = GetHitboxPositionAtTime(HitboxIDB, CurrentTime - PingTime);
	//We multiply the distance the hitboxes can be apart by a multiplier because ping isn't 100% accurate and we are also estimating based on lerping for position.
	return FVector::DistSquared(PositionA, PositionB)
		< FMath::Square((HitboxA->GetScaledSphereRadius() + HitboxB->GetScaledSphereRadius()) * HitboxToleranceMultiplier);
}