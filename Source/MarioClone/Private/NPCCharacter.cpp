#include "NPCCharacter.h"
#include "HealthComponent.h"
#include "Hitbox.h"
#include "PaperFlipbookComponent.h"
#include "GameFramework/PawnMovementComponent.h"

ANPCCharacter::ANPCCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	GetSprite()->SetUsingAbsoluteRotation(true);
	GetSprite()->SetWorldRotation(FRotator(0.0f));
	
	Hitbox = CreateDefaultSubobject<UHitbox>(FName(TEXT("Hitbox")));
	Hitbox->SetupAttachment(RootComponent);
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	HealthComponent->SetupAttachment(RootComponent);
}

void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//Constrain movement to the X and Z axes, since this is a 2D game.
	GetMovementComponent()->SetPlaneConstraintEnabled(true);
	GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Y);

	HitboxCallback.BindDynamic(this, &ANPCCharacter::OnHitboxCollision);
	Hitbox->SubscribeToHitboxCollision(HitboxCallback);
}

void ANPCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANPCCharacter::OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue)
{
	if (HasAuthority())
	{
		if (!BounceImpulse.IsNearlyZero())
		{
			LaunchCharacter(BounceImpulse, false, true);
		}
		if (DamageValue != 0.0f)
		{
			HealthComponent->ModifyHealth(DamageValue * -1.0f);
		}
	}
}



