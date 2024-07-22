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

bool UHitboxManager::SanityCheckBounce(const int32 HitboxIDA, const int32 HitboxIDB) const
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
	//On the server we will give a lot of leeway so that latency doesn't prevent us from bouncing.
	//This is a trade-off between cheat-proofing accuracy and latency tolerance. A rewinding system would be better but out of scope for now.
	const float ToleranceMultiplier = GetWorld()->GetNetMode() < NM_Client ? 2.0f : 1.0f;
	//There is additionally some tolerance built in here because we just get the bounds radius, which is the widest possible distance these two boxes could be from each other without regard for direction.
	return FVector::DistSquared(HitboxA->GetComponentLocation(), HitboxB->GetComponentLocation())
		< FMath::Square((HitboxA->Bounds.SphereRadius + HitboxB->Bounds.SphereRadius) * ToleranceMultiplier);
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