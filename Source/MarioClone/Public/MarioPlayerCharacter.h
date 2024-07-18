#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MarioPlayerCharacter.generated.h"

class UCameraComponent;

UCLASS()
class MARIOCLONE_API AMarioPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

#pragma region Core

public:
	
	AMarioPlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float MaxCameraLeadOffset = 400.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraLeadInterpSpeed = 300.0f;

	FVector DesiredBaseCameraOffset;

#pragma endregion 
#pragma region Movement

public:

private:

	UFUNCTION()
	void MovementInput(const float AxisValue);
	UFUNCTION()
	void JumpPressed();
	UFUNCTION()
	void JumpReleased();
	
};