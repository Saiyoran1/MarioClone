#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "Hitbox.h"
#include "PaperCharacter.h"
#include "NPCCharacter.generated.h"

UCLASS()
class MARIOCLONE_API ANPCCharacter : public APaperCharacter, public ICombatInterface
{
	GENERATED_BODY()

public:
	
	ANPCCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual EHostility GetHostility_Implementation() const override { return Hostility; }
	virtual UHealthComponent* GetHealthComponent_Implementation() const override { return HealthComponent; } 

private:

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHitbox* Hitbox = nullptr;
	FHitboxCallback HitboxCallback;
	UFUNCTION()
	void OnHitboxCollision(UHitbox* CollidingHitbox, const FVector& BounceImpulse, const float DamageValue);
	
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Hostility")
	EHostility Hostility = EHostility::Enemy;
};