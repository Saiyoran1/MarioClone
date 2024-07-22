#include "Hitbox.h"
#include "HitboxManager.h"
#include "Net/UnrealNetwork.h"

const FName UHitbox::HitboxProfile = FName(TEXT("Hitbox"));

#pragma region Core

UHitbox::UHitbox()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UHitbox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHitbox, HitboxID);
}

void UHitbox::InitializeComponent()
{
	Super::InitializeComponent();

	//InitializeComponent also runs in the editor when loading the owning actor.
	//Check here that we are actually in the game.
	if (!IsValid(GetWorld()) || !GetWorld()->IsGameWorld())
	{
		return;
	}

	SetCollisionProfileName(HitboxProfile);
	OnComponentBeginOverlap.AddDynamic(this, &UHitbox::OnOverlap);
}

void UHitbox::BeginPlay()
{
	Super::BeginPlay();
	//Cache pawn owner to check for locally controlled when doing bounce collisions.
	OwnerAsPawn = Cast<APawn>(GetOwner());
	//Save off hitbox hostility for determining collision behavior with other hitboxes.
	//Defaults to Neutral for actors not implementing the interface.
	if (GetOwner()->Implements<UCombatInterface>())
	{
		OwnerHostility = ICombatInterface::Execute_GetHostility(GetOwner());
	}
	//Assign a unique ID to this hitbox, used for networking bounces and prioritizing hitboxes during collisions.
	if (GetOwner()->HasAuthority())
	{
		UHitboxManager* HitboxManager = GetWorld()->GetSubsystem<UHitboxManager>();
		if (IsValid(HitboxManager))
		{
			HitboxID = HitboxManager->RegisterNewHitbox(this);
		}
	}
}

void UHitbox::OnRep_HitboxID()
{
	if (HitboxID == -1)
	{
		return;
	}
	UHitboxManager* HitboxManager = GetWorld()->GetSubsystem<UHitboxManager>();
	if (IsValid(HitboxManager))
	{
		HitboxManager->RegisterNewHitbox(this, HitboxID);
	}
}

#pragma endregion
#pragma region Collision

void UHitbox::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//We ignore collisions of hitboxes against other things that aren't hitboxes.
	//This is already partially handled by the hitbox profiles themselves (which only overlap other hitboxes),
	//but its possible to set something else in the world to use one of those profiles or object channels by mistake.
	//In that case, we just ignore the event here, because we have to cast to UHitbox anyway to get bounce and damage info.
	UHitbox* CollidingHitbox = Cast<UHitbox>(OtherComp);
	if (!IsValid(CollidingHitbox))
	{
		return;
	}
	//When two hitboxes collide, only one of them actually processes the collision.
	//The other will be notified by the one that does the processing.
	if (!ShouldProcessCollision(CollidingHitbox))
	{
		return;
	}
	//Calculate the bounce impulse and damage to apply to each hitbox.
	FVector ImpulseToThis = FVector::ZeroVector;
	FVector ImpulseToOther = FVector::ZeroVector;
	float DamageToThis = 0.0f;
	float DamageToOther = 0.0f;
	ProcessCollision(CollidingHitbox, ImpulseToThis, ImpulseToOther, DamageToThis, DamageToOther);
	//Broadcast the collision result, and tell the other hitbox to broadcast it as well.
	OnHitboxCollision.Broadcast(CollidingHitbox, ImpulseToThis, DamageToThis);
	CollidingHitbox->NotifyOfCollisionResult(this, ImpulseToOther, DamageToOther);
}

bool UHitbox::ShouldProcessCollision(UHitbox* OtherHitbox) const
{
	//If two hitboxes are the same hostility, we won't process their collision.
	if (OtherHitbox->GetHostility() == OwnerHostility)
	{
		return false;
	}
	//Otherwise, friendly > enemy > neutral for priority on who calculates collisions.
	return OtherHitbox->GetHostility() < OwnerHostility;
}

void UHitbox::ProcessCollision(UHitbox* OtherHitbox, FVector& ImpulseToThis, FVector& ImpulseToOther, float& DamageToThis, float& DamageToOther) const
{
	//Check if this hitbox is above the threshold to deal damage and receive a bounce from the other hitbox.
	const float ThisHitboxMinZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
	bool bUseOtherHitboxThreshold = true;
	const float OtherHitboxThreshold = OtherHitbox->GetCollisionThreshold(bUseOtherHitboxThreshold);

	if (!bUseOtherHitboxThreshold || ThisHitboxMinZ > OtherHitboxThreshold)
	{
		if (DealsCollisionDamage() && OtherHitbox->CanBeCollisionDamaged())
		{
			DamageToOther = GetCollisionDamageDone();
		}
		if (CanBeBounced() && OtherHitbox->IsBouncy())
		{
			ImpulseToThis = OtherHitbox->GetBounceImpulse();
		}
	}
	//If we were below the threshold, we will instead receive damage and potentially bounce the other hitbox.
	else
	{
		if (CanBeCollisionDamaged() && OtherHitbox->DealsCollisionDamage())
		{
			DamageToThis = OtherHitbox->GetCollisionDamageDone();
		}
		if (IsBouncy() && OtherHitbox->CanBeBounced())
		{
			ImpulseToOther = GetBounceImpulse();
		}
	}
}

bool UHitbox::CanBeCollisionDamaged() const
{
	return GetOwnerRole() == ROLE_Authority && bCanBeCollisionDamaged;
}

bool UHitbox::CanBeBounced() const
{
	return bCanBeBounced && (IsValid(OwnerAsPawn) ? OwnerAsPawn->IsLocallyControlled() : GetOwnerRole() == ROLE_Authority);
}

void UHitbox::NotifyOfCollisionResult(UHitbox* CollidingHitbox, const FVector& Bounce, const float Damage)
{
	OnHitboxCollision.Broadcast(CollidingHitbox, Bounce, Damage);
}

void UHitbox::SubscribeToHitboxCollision(const FHitboxCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnHitboxCollision.AddUnique(Callback);
	}
}

void UHitbox::UnsubscribeFromHitboxCollision(const FHitboxCallback& Callback)
{
	if (Callback.IsBound())
	{
		OnHitboxCollision.Remove(Callback);
	}
}

#pragma endregion 