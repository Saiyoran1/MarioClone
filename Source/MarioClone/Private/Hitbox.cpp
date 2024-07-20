#include "Hitbox.h"
#include "CombatInterface.h"

const FName UHitbox::FriendlyHitboxProfile = FName(TEXT("FriendlyHitbox"));
const FName UHitbox::EnemyHitboxProfile = FName(TEXT("EnemyHitbox"));
const FName UHitbox::NeutralHitboxProfile = FName(TEXT("NeutralHitbox"));

#pragma region Core

UHitbox::UHitbox()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UHitbox::InitializeComponent()
{
	Super::InitializeComponent();

	if (GetOwner()->Implements<UCombatInterface>())
	{
		switch (ICombatInterface::Execute_GetHostility(GetOwner()))
		{
		case EHostility::Neutral :
			SetCollisionProfileName(NeutralHitboxProfile);
			break;
		case EHostility::Friendly :
			SetCollisionProfileName(FriendlyHitboxProfile);
			break;
		case EHostility::Enemy :
			SetCollisionProfileName(EnemyHitboxProfile);
			break;
		default :
			SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		SetCollisionProfileName(NeutralHitboxProfile);
	}
	OnComponentBeginOverlap.AddDynamic(this, &UHitbox::OnOverlap);
}

void UHitbox::BeginPlay()
{
	Super::BeginPlay();
	//Cache off pawn owner to check for locally controlled when doing bounce collisions.
	OwnerAsPawn = Cast<APawn>(GetOwner());
}

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
	FVector BounceVector = FVector::ZeroVector;
	if (CanBeBounced())
	{
		BounceVector = CollidingHitbox->GetBounceImpulse(this);
	}
	float DamageValue = 0.0f;
	if (CanBeCollisionDamaged())
	{
		DamageValue = CollidingHitbox->GetCollisionDamageDone(this);
	}
	OnHitboxCollision.Broadcast(CollidingHitbox, BounceVector, DamageValue);
}

bool UHitbox::IsHitboxAboveThreshold(const float Threshold, const UHitbox* CollidingHitbox) const
{
	const float CollidingHitboxMinZ = CollidingHitbox->Bounds.Origin.Z - CollidingHitbox->Bounds.BoxExtent.Z;
	const float HitboxMinZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
	const float HitboxMaxZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;
	const float ZThreshold = FMath::Lerp(HitboxMinZ, HitboxMaxZ, FMath::Clamp(Threshold, 0.0f, 1.0f));
	const bool bAboveThreshold = CollidingHitboxMinZ > ZThreshold;

	const FColor DEBUG_Color = bAboveThreshold ? FColor::Red : FColor::Yellow;
	DrawDebugBox(GetWorld(), Bounds.Origin, Bounds.BoxExtent, FColor::Green, false, 2.0f, 0, 2);
	DrawDebugBox(GetWorld(), CollidingHitbox->Bounds.Origin, CollidingHitbox->Bounds.BoxExtent, DEBUG_Color, false,2.0f, 0, 2);
	DrawDebugLine(GetWorld(), FVector(CollidingHitbox->GetComponentLocation().X, CollidingHitbox->GetComponentLocation().Y, CollidingHitboxMinZ),
		FVector(GetComponentLocation().X, GetComponentLocation().Y, ZThreshold), DEBUG_Color, false,2.0f, 0, 2);
	
	return bAboveThreshold;
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
#pragma region Bounce

bool UHitbox::CanBeBounced() const
{
	//If bouncing is disabled, no need to check owner role.
	if (!bCanBeBounced)
	{
		return false;
	}
	//Pawns should only be bounced if they're locally controlled, since bouncing will need client prediction.
	if (IsValid(OwnerAsPawn))
	{
		return OwnerAsPawn->IsLocallyControlled();
	}
	//If the owning actor isn't a pawn, we can bounce without care for any kind of prediction.
	return true;
}

FVector UHitbox::GetBounceImpulse(const UHitbox* CollidingHitbox) const
{
	if (!bIsBouncy)
	{
		return FVector::ZeroVector;
	}
	bool bThresholdMet = true;
	switch (BounceThresholdRequirement)
	{
	case EThresholdRequirement::Above :
		bThresholdMet = IsHitboxAboveThreshold(BounceThreshold, CollidingHitbox);
		break;
	case EThresholdRequirement::Below :
		bThresholdMet = !IsHitboxAboveThreshold(BounceThreshold, CollidingHitbox);
		break;
	default :
		break;
	}
	if (!bThresholdMet)
	{
		return FVector::ZeroVector;
	}
	return BaseBounceImpulse;
}

#pragma endregion
#pragma region Damage

bool UHitbox::CanBeCollisionDamaged() const
{
	//Damage should only happen on the server for replicated actors.
	return bCanBeCollisionDamaged && GetOwnerRole() == ROLE_Authority;
}

float UHitbox::GetCollisionDamageDone(const UHitbox* CollidingHitbox) const
{
	if (!bDealsCollisionDamage)
	{
		return 0.0f;
	}
	bool bThresholdMet = true;
	switch (CollisionDamageThresholdRequirement)
	{
	case EThresholdRequirement::Above :
		bThresholdMet = IsHitboxAboveThreshold(DamageThreshold, CollidingHitbox);
		break;
	case EThresholdRequirement::Below :
		bThresholdMet = !IsHitboxAboveThreshold(DamageThreshold, CollidingHitbox);
		break;
	default :
		break;
	}
	if (!bThresholdMet)
	{
		return 0.0f;
	}
	return BaseCollisionDamage;
}

#pragma endregion 