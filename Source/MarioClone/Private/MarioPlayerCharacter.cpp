#include "MarioClone/Public/MarioPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PawnMovementComponent.h"

AMarioPlayerCharacter::AMarioPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("Camera")));
	Camera->SetupAttachment(RootComponent);
	Camera->SetUsingAbsoluteRotation(true);
	Camera->SetWorldRotation(FRotator(0.0f));
	Camera->SetRelativeLocation(FVector(0.0f, 800.0f, 100.0f));
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
}

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

void AMarioPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		//Update camera location to lead the character when moving.
		const FVector LastMoveInput = GetMovementComponent()->GetLastInputVector();
		const FVector DesiredOffset = FVector(DesiredBaseCameraOffset.X, DesiredBaseCameraOffset.Y + (LastMoveInput.Y * MaxCameraLeadOffset), DesiredBaseCameraOffset.Z);
		const FVector DesiredFinalLocation = GetActorLocation() + DesiredOffset;
		Camera->SetWorldLocation(FMath::VInterpConstantTo(Camera->GetComponentLocation(), DesiredFinalLocation, DeltaTime, CameraLeadInterpSpeed));
	}
}