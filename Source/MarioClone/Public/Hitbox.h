#pragma once
#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "Hitbox.generated.h"

class UHitbox;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FHitboxCallback, UHitbox*, CollidingHitbox, const FVector&, BounceImpulse, const float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHitboxNotification, UHitbox*, CollidingHitbox, const FVector&, BounceImpulse, const float, DamageAmount);

UENUM()
enum class EThresholdRequirement : uint8
{
	Either,
	Above,
	Below
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MARIOCLONE_API UHitbox : public UCapsuleComponent
{
	GENERATED_BODY()

#pragma region Core

public:
	
	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;

	void SubscribeToHitboxCollision(const FHitboxCallback& Callback);
	void UnsubscribeFromHitboxCollision(const FHitboxCallback& Callback);

private:

	static const FName FriendlyHitboxProfile;
	static const FName EnemyHitboxProfile;
	static const FName NeutralHitboxProfile;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FHitboxNotification OnHitboxCollision;
	
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	
	//Helper function for determining whether a colliding component is above the CollisionEffectZThreshold.
	bool IsHitboxAboveThreshold(const float Threshold, const UHitbox* CollidingHitbox) const;

#pragma endregion 
#pragma region Bounce

public:

	bool CanBeBounced() const;
	FVector GetBounceImpulse(const UHitbox* CollidingHitbox) const;

private:

	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce")
	bool bIsBouncy = true;
	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce", meta = (EditCondition = "bIsBouncy"))
	FVector BaseBounceImpulse = FVector(0.0f, 0.0f, 1000.0f);
	//Determines when to apply bounce impulses during collision.
	//If set to Above, for example, only bounce hitboxes above the BounceThreshold.
	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce", meta = (EditCondition = "bIsBouncy"))
	EThresholdRequirement BounceThresholdRequirement = EThresholdRequirement::Above;
	//The threshold, as a proportion of the hitbox's z extent, at which to measure whether a colliding hitbox is "above" or "below" for bounce purposes.
	//Ex: If this hitbox is 100 units tall, and the BounceThreshold is set to 0.7f, then the bottom of the colliding hitbox would have to be at least
	//70 units above the bottom of this hitbox to be considered "above" the threshold.
	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce",
		meta = (EditCondition = "bIsBouncy && BounceThresholdRequirement != EThresholdRequirement::Either",
		ClampMin = "0", ClampMax = "1"))
	float BounceThreshold = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce")
	bool bCanBeBounced = false;

#pragma endregion
#pragma region Damage

public:

	bool CanBeCollisionDamaged() const;
	float GetCollisionDamageDone(const UHitbox* CollidingHitbox) const;

private:

	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage")
	bool bDealsCollisionDamage = true;
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage", meta = (EditCondition = "bDealsCollisionDamage"))
	float BaseCollisionDamage = 50.0f;
	//Determines when to apply damage during collision.
	//If set to Below, for example, only damage hitboxes below the DamageThreshold.
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage", meta = (EditCondition = "bDealsCollisionDamage"))
	EThresholdRequirement CollisionDamageThresholdRequirement = EThresholdRequirement::Below;
	//The threshold, as a proportion of the hitbox's z extent, at which to measure whether a colliding hitbox is "above" or "below" for damage purposes.
	//Ex: If this hitbox is 100 units tall, and the DamageThreshold is set to 0.7f, then the bottom of the colliding hitbox would have to be at least
	//70 units above the bottom of this hitbox to be considered "above" the threshold.
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage",
		meta = (EditCondition = "bDealsCollisionDamage && CollisionDamageThresholdRequirement != EThresholdRequirement::Either",
		ClampMin = "0", ClampMax = "1"))
	float DamageThreshold = 0.7f;
	
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage")
	bool bCanBeCollisionDamaged = true;

#pragma endregion 
};