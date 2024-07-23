#include "HitboxManager.h"
#include "Hitbox.h"

void UHitboxManager::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	HitboxMap.Empty();
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
	//We will give a lot of leeway so that latency doesn't prevent us from bouncing.
	//This is a trade-off between cheat-proofing accuracy and latency tolerance. A rewinding system would be better but out of scope for now.
	const float ToleranceMultiplier = 3.0f;
	//We also add a bit more leeway in the form of checking velocity of both actors. If they were moving in opposite directions, its possible that the distance they've moved while the RPC was sent is large.
	//This is generous in that it always assumes the hitboxes were moving away from each other at whatever speed they're travelling right now.
	const float PossibleMoveDistance = (HitboxA->GetOwner()->GetVelocity() * PingTime).Size() + (HitboxB->GetOwner()->GetVelocity() * PingTime).Size();
	//There is additionally some tolerance built in here because we just get the bounds radius, which is the widest possible distance these two boxes could be from each other without regard for direction.
	return FVector::DistSquared(HitboxA->GetComponentLocation(), HitboxB->GetComponentLocation())
		< FMath::Square((HitboxA->Bounds.SphereRadius + HitboxB->Bounds.SphereRadius + PossibleMoveDistance) * ToleranceMultiplier);
}

int32 UHitboxManager::RegisterNewHitbox(const UHitbox* Hitbox)
{
	if (!IsValid(Hitbox))
	{
		return -1;
	}
	const int32 NewID = HitboxIDCounter++;
	HitboxMap.Add(NewID, Hitbox);
	return NewID;
}

void UHitboxManager::RegisterNewHitbox(const UHitbox* Hitbox, const int32 ID)
{
	if (ID == -1 || !IsValid(Hitbox))
	{
		return;
	}
	HitboxMap.Add(ID, Hitbox);
}