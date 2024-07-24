#pragma once
#include "CoreMinimal.h"
#include "CombatInterface.h"
#include "Components/SphereComponent.h"
#include "Hitbox.generated.h"

class UHitbox;

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FHitboxCallback, UHitbox*, CollidingHitbox, const FVector&, BounceImpulse, const float, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHitboxNotification, UHitbox*, CollidingHitbox, const FVector&, BounceImpulse, const float, DamageAmount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MARIOCLONE_API UHitbox : public USphereComponent
{
	GENERATED_BODY()

#pragma region Core

public:
	
	UHitbox();
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	EHostility GetHostility() const { return OwnerHostility; }
	int32 GetHitboxID() const { return HitboxID; }

private:
	
	UPROPERTY()
	APawn* OwnerAsPawn = nullptr;
	EHostility OwnerHostility = EHostility::Neutral;
	//This unique ID is used to identify hitboxes across the network to verify bounces on the server.
	UPROPERTY(ReplicatedUsing = OnRep_HitboxID)
	int32 HitboxID = -1;
	//This registers the hitbox with the local HitboxManager, ensuring that all clients have an accurate map of bounce boxes to IDs for bouncing.
	UFUNCTION()
	void OnRep_HitboxID();

#pragma endregion
#pragma region Collision

public:

	void EnableHitbox();
	void DisableHitbox();
	
	float GetCollisionThreshold(bool& bOutUseThreshold) const;
	
	bool IsBouncy() const { return bIsBouncy; }
	bool CanBeBounced() const;
	FVector GetBounceImpulse() const { return BounceImpulse; }
	
	bool DealsCollisionDamage() const { return bDealsCollisionDamage; }
	bool CanBeCollisionDamaged() const;
	float GetCollisionDamageDone() const { return CollisionDamage; }

	//Called from another hitbox that handled a collision with this hitbox.
	void NotifyOfCollisionResult(UHitbox* CollidingHitbox, const FVector& Bounce, const float Damage);

	void SubscribeToHitboxCollision(const FHitboxCallback& Callback);
	void UnsubscribeFromHitboxCollision(const FHitboxCallback& Callback);

private:

	static const FName HitboxProfile;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	bool bUseCollisionThreshold = true;
	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "bUseCollisionThreshold"))
	float CollisionThreshold = 0.75;

	//Callback from native component OnBeginOverlap for this hitbox.
	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	//Since two hitboxes colliding triggers two overlap events (one for each hitbox), this function can deterministically pick one hitbox to actually perform calculations.
	//The hitbox that isn't selected will receive the result of the collision from the hitbox that is selected.
	//It can also just opt to not perform calculations at all for hitboxes of the same team.
	bool ShouldProcessCollision(UHitbox* OtherHitbox) const;
	//Performs the actual bounce impulse and damage value calculations based on hitbox locations.
	void ProcessCollision(UHitbox* OtherHitbox, FVector& ImpulseToThis, FVector& ImpulseToOther, float& DamageToThis, float& DamageToOther) const;
	//Delegate called when a collision is processed for this hitbox with the resulting bounce and damage info.
	FHitboxNotification OnHitboxCollision;

	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce")
	bool bIsBouncy = true;
	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce", meta = (EditCondition = "bIsBouncy"))
	FVector BounceImpulse = FVector(0.0f, 0.0f, 1000.0f);
	UPROPERTY(EditAnywhere, Category = "Hitbox|Bounce")
	bool bCanBeBounced = false;
	
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage")
	bool bDealsCollisionDamage = true;
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage", meta = (EditCondition = "bDealsCollisionDamage"))
	float CollisionDamage = 50.0f;
	UPROPERTY(EditAnywhere, Category = "Hitbox|Damage")
	bool bCanBeCollisionDamaged = true;

#pragma endregion 
};