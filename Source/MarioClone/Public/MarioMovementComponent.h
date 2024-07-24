#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MarioMovementComponent.generated.h"

class UHitboxManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MARIOCLONE_API UMarioMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

#pragma region SavedMove

	struct FSavedMove_Mario : public FSavedMove_Character
	{
	public:
		
		typedef FSavedMove_Character Super;
		
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
		virtual void Clear() override;
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual uint8 GetCompressedFlags() const override;

		uint8 bSavedWantsBounce : 1;
		int32 SavedThisBounceBoxID = -1;
		int32 SavedOtherBounceBoxID = -1;
		bool bSavedBouncedThis = false;
		bool bSavedBouncedOther = false;
		bool bSavedDamagedThis = false;
		bool bSavedDamagedOther = false;
	};

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

#pragma endregion
#pragma region ClientPredictionData

	class FNetworkPredictionData_Client_Mario : public FNetworkPredictionData_Client_Character
	{
	public:

		typedef FNetworkPredictionData_Client_Character Super;

		FNetworkPredictionData_Client_Mario(const UCharacterMovementComponent& ClientMovement);
		virtual FSavedMovePtr AllocateNewMove() override;
	};

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

#pragma endregion
#pragma region NetworkMoveData

	struct FMarioNetworkMoveData : public FCharacterNetworkMoveData
	{
	public:

		typedef FCharacterNetworkMoveData Super;

		int32 ThisBounceBoxID = -1;
		int32 OtherBounceBoxID = -1;
		bool bBouncedThis = false;
		bool bBouncedOther = false;
		bool bDamagedThis = false;
		bool bDamagedOther = false;

		virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
		virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
	};

	struct FMarioNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
	{
	public:

		typedef FCharacterNetworkMoveDataContainer Super;

		FMarioNetworkMoveData CustomMoves[3];

		FMarioNetworkMoveDataContainer()
		{
			NewMoveData = &CustomMoves[0];
			PendingMoveData = &CustomMoves[1];
			OldMoveData = &CustomMoves[2];
		}
	};

private:

	FMarioNetworkMoveDataContainer MarioMoveDataContainer;

#pragma endregion 

public:

	UMarioMovementComponent();
	virtual void BeginPlay() override;
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual float GetGravityZ() const override;
	
	void OnHitboxCollision(const int32 ThisID, const int32 OtherID,
		const bool bThisBounced, const bool bOtherBounced, const bool bThisDamaged, const bool bOtherDamaged);

private:

	//Multiplier on gravity when velocity is downward, to make jumping and falling faster.
	UPROPERTY(EditDefaultsOnly, Category = "Custom Movement")
	float DownwardGravityMultiplier = 2.0f;

	UPROPERTY()
	UHitboxManager* HitboxManager = nullptr;
	uint8 bWantsBounce : 1;
	int32 ThisHitboxID = -1;
	int32 OtherHitboxID = -1;
	bool bBouncedThis = false;
	bool bBouncedOther = false;
	bool bDamagedThis = false;
	bool bDamagedOther = false;
};