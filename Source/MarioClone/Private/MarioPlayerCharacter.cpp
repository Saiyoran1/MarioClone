#include "MarioClone/Public/MarioPlayerCharacter.h"
#include "HealthComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PawnMovementComponent.h"

const FName AMarioPlayerCharacter::HitboxCollisionProfile = FName(TEXT("Hitbox"));

AMarioPlayerCharacter::AMarioPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("Camera")));
	Camera->SetupAttachment(RootComponent);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetWorldRotation(FRotator(0.0f));
	Camera->SetRelativeLocation(FVector(0.0f, 800.0f, 100.0f));

	Hitbox = CreateDefaultSubobject<UCapsuleComponent>(FName(TEXT("Hitbox")));
	Hitbox->SetupAttachment(RootComponent);
	Hitbox->SetCollisionProfileName(HitboxCollisionProfile);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(FName(TEXT("Health")));
	HealthComponent->SetupAttachment(RootComponent);
}

void AMarioPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	//Constrain movement to the Y and Z axes, since this is a 2D game.
	GetMovementComponent()->SetPlaneConstraintEnabled(true);
	GetMovementComponent()->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::X);

	//Bind to the health and life status delegates of the health component.
	HealthCallback.BindDynamic(this, &AMarioPlayerCharacter::OnHealthChanged);
	LifeCallback.BindDynamic(this, &AMarioPlayerCharacter::OnLifeStatusChanged);
	HealthComponent->SubscribeToHealthChanged(HealthCallback);
	HealthComponent->SubscribeToLifeStatusChanged(LifeCallback);

	//Bind to hitbox overlaps to deal damage and bounce.
	Hitbox->OnComponentBeginOverlap.AddDynamic(this, &AMarioPlayerCharacter::OnHitboxOverlap);
}

void AMarioPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled())
	{
		//Cache off camera's initial world space offset, so that we can use it as a reference point when leading the player.
		DesiredBaseCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	}
}

void AMarioPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(FName("Movement"), this, &AMarioPlayerCharacter::MovementInput);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &AMarioPlayerCharacter::JumpPressed);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Released, this, &AMarioPlayerCharacter::JumpReleased);

#if WITH_EDITOR
	//Debug keybindings for testing
	PlayerInputComponent->BindAction(FName("DEBUG_DamageSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_DamageSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_HealSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_HealSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_KillSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_KillSelf);
	PlayerInputComponent->BindAction(FName("DEBUG_ReviveSelf"), IE_Pressed, this, &AMarioPlayerCharacter::DEBUG_ReviveSelf);
#endif
}

void AMarioPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		//Update camera location to lead the character when moving.
		const float LastMoveInputY = GetMovementComponent()->GetLastInputVector().Y;
		//If we aren't moving, we'll update how long we haven't been moving for.
		if (LastMoveInputY == 0.0f)
		{
			TimeAtRest += DeltaTime;
			//After a delay, we'll recenter the camera on the character.
			if (TimeAtRest > CameraRecenterDelay)
			{
				Camera->SetWorldLocation(FMath::VInterpConstantTo(Camera->GetComponentLocation(), GetActorLocation() + DesiredBaseCameraOffset, DeltaTime, CameraRecenterSpeed));
			}
		}
		//If we are moving, the camera should lead the character along the Y axis.
		else
		{
			TimeAtRest = 0.0f;
			const FVector DesiredOffset = FVector(DesiredBaseCameraOffset.X, DesiredBaseCameraOffset.Y + (LastMoveInputY * MaxCameraLeadOffset), DesiredBaseCameraOffset.Z);
			const FVector DesiredFinalLocation = GetActorLocation() + DesiredOffset;
			Camera->SetWorldLocation(FMath::VInterpConstantTo(Camera->GetComponentLocation(), DesiredFinalLocation, DeltaTime, CameraLeadInterpSpeed));
		}
	}

	DrawDebugString(GetWorld(), GetActorLocation() + FVector::UpVector * 100.0f,
		FString::Printf(L"%i/%i", FMath::RoundToInt(HealthComponent->GetCurrentHealth()), FMath::RoundToInt(HealthComponent->GetMaxHealth())), 0, FColor::Red, 0);
}

#pragma region Movement

void AMarioPlayerCharacter::MovementInput(const float AxisValue)
{
	GetMovementComponent()->AddInputVector(FVector(0.0f, 1.0f, 0.0f) * AxisValue);
}

void AMarioPlayerCharacter::JumpPressed()
{
	Jump();
}

void AMarioPlayerCharacter::JumpReleased()
{
	//TODO: Float/mana/drop attack.
}

#pragma endregion 
#pragma region Health

void AMarioPlayerCharacter::OnHealthChanged(const float PreviousHealth, const float CurrentHealth, const float MaxHealth)
{
	//If we took damage, we need to reset our health regen timer and become briefly immune to damage.
	if (CurrentHealth < PreviousHealth)
	{
		//TODO: Server health regeneration
		//TODO: All machines, play a hit reaction (if we didn't hit 0 health here).
	}
}

void AMarioPlayerCharacter::OnLifeStatusChanged(const bool bNewLifeStatus)
{
	//TODO: Play death animation, stop input, timer for respawn?
}

#pragma endregion
#pragma region Collision

void AMarioPlayerCharacter::OnHitboxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	const FHitResult& SweepResult)
{
	const float HitboxMinZ = OverlappedComponent->Bounds.Origin.Z - OverlappedComponent->Bounds.BoxExtent.Z;
	const float OtherHitboxOriginZ = OtherComp->Bounds.Origin.Z;
	const float OtherHitboxMaxZ = OtherHitboxOriginZ + OtherComp->Bounds.BoxExtent.Z;
	//Arbitrary threshold for determining if we are "above" or "below" and can deal damage or not.
	const float ZThreshold = FMath::Lerp(OtherHitboxOriginZ, OtherHitboxMaxZ, 0.5f);
	const bool bAboveThreshold = HitboxMinZ > ZThreshold;
	if (bAboveThreshold)
	{
		if (IsLocallyControlled())
		{
			LaunchCharacter(FVector::UpVector * 1000.0f, false, true);
		}
		//On the server we can try to do damage to enemies if we are landing on them from above.
		if (HasAuthority())
		{
			if ( OtherActor->Implements<UCombatInterface>() && ICombatInterface::Execute_IsEnemy(OtherActor))
			{
				UHealthComponent* EnemyHealth = ICombatInterface::Execute_GetHealthComponent(OtherActor);
				if (IsValid(EnemyHealth))
				{
					EnemyHealth->ModifyHealth(BaseDamage * -1.0f);
				}
			}
		}
	}
	const FColor DEBUG_Color = bAboveThreshold ? FColor::Red : FColor::Yellow;
	DrawDebugBox(GetWorld(), OverlappedComponent->Bounds.Origin, OverlappedComponent->Bounds.BoxExtent, FColor::Green, false, 2.0f, 0, 2);
	DrawDebugBox(GetWorld(), OtherComp->Bounds.Origin, OtherComp->Bounds.BoxExtent, DEBUG_Color, false,2.0f, 0, 2);
	DrawDebugLine(GetWorld(), FVector(OverlappedComponent->GetComponentLocation().X, OverlappedComponent->GetComponentLocation().Y, HitboxMinZ),
		FVector(OtherComp->GetComponentLocation().X, OtherComp->GetComponentLocation().Y, ZThreshold), DEBUG_Color, false,2.0f, 0, 2);
}

#pragma endregion 

#if WITH_EDITOR
#pragma region Debug

void AMarioPlayerCharacter::DEBUG_DamageSelf()
{
	Server_DEBUG_DamageSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_DamageSelf_Implementation()
{
	HealthComponent->ModifyHealth(-5.0f);
}

void AMarioPlayerCharacter::DEBUG_HealSelf()
{
	Server_DEBUG_HealSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_HealSelf_Implementation()
{
	HealthComponent->ModifyHealth(3.5f);
}

void AMarioPlayerCharacter::DEBUG_KillSelf()
{
	Server_DEBUG_KillSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_KillSelf_Implementation()
{
	HealthComponent->InstantKill();
}

void AMarioPlayerCharacter::DEBUG_ReviveSelf()
{
	Server_DEBUG_ReviveSelf();
}

void AMarioPlayerCharacter::Server_DEBUG_ReviveSelf_Implementation()
{
	HealthComponent->ResetHealth();
}

#pragma endregion
#endif