#include "MarioMovementComponent.h"
#include "HitboxManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"

#pragma region SavedMove

void UMarioMovementComponent::FSavedMove_Mario::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	const UMarioMovementComponent* MovementComponent = Cast<UMarioMovementComponent>(C->GetCharacterMovement());
	if (IsValid(MovementComponent))
	{
		bSavedWantsBounce = MovementComponent->bWantsBounce;
		SavedThisBounceBoxID = MovementComponent->ThisHitboxID;
		SavedOtherBounceBoxID = MovementComponent->OtherHitboxID;
		bSavedBouncedThis = MovementComponent->bBouncedThis;
		bSavedBouncedOther = MovementComponent->bBouncedOther;
		bSavedDamagedThis = MovementComponent->bDamagedThis;
		bSavedDamagedOther = MovementComponent->bDamagedOther;
	}
}

void UMarioMovementComponent::FSavedMove_Mario::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);

	UMarioMovementComponent* MovementComponent = Cast<UMarioMovementComponent>(C->GetCharacterMovement());
	if (IsValid(MovementComponent))
	{
		MovementComponent->bWantsBounce = bSavedWantsBounce;
		MovementComponent->ThisHitboxID = SavedThisBounceBoxID;
		MovementComponent->OtherHitboxID = SavedOtherBounceBoxID;
		MovementComponent->bBouncedThis = bSavedBouncedThis;
		MovementComponent->bBouncedOther = bSavedBouncedOther;
		MovementComponent->bDamagedThis = bSavedDamagedThis;
		MovementComponent->bDamagedOther = bSavedDamagedOther;
	}
}

void UMarioMovementComponent::FSavedMove_Mario::Clear()
{
	Super::Clear();
	bSavedWantsBounce = false;
	SavedThisBounceBoxID = -1;
	SavedOtherBounceBoxID = -1;
	bSavedBouncedThis = false;
	bSavedBouncedOther = false;
	bSavedDamagedThis = false;
	bSavedDamagedOther = false;
}

bool UMarioMovementComponent::FSavedMove_Mario::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	if (!Super::CanCombineWith(NewMove, InCharacter, MaxDelta))
	{
		return false;
	}
	const FSavedMove_Mario* NewMoveCast = static_cast<FSavedMove_Mario*>(NewMove.Get());
	return bSavedWantsBounce == NewMoveCast->bSavedWantsBounce
		&& SavedThisBounceBoxID == NewMoveCast->SavedThisBounceBoxID
		&& SavedOtherBounceBoxID == NewMoveCast->SavedOtherBounceBoxID
		&& bSavedBouncedThis == NewMoveCast->bSavedBouncedThis
		&& bSavedBouncedOther == NewMoveCast->bSavedBouncedOther
		&& bSavedDamagedThis == NewMoveCast->bSavedDamagedThis
		&& bSavedDamagedOther == NewMoveCast->bSavedDamagedOther;
}

uint8 UMarioMovementComponent::FSavedMove_Mario::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();
	if (bSavedWantsBounce)
	{
		Flags |= FLAG_Custom_0;
	}
	return Flags;
}

void UMarioMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsBounce = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

#pragma endregion
#pragma region ClientPredictionData

UMarioMovementComponent::FNetworkPredictionData_Client_Mario::FNetworkPredictionData_Client_Mario(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UMarioMovementComponent::FNetworkPredictionData_Client_Mario::AllocateNewMove()
{
	return MakeShared<FSavedMove_Mario>(FSavedMove_Mario());
}

FNetworkPredictionData_Client* UMarioMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		UMarioMovementComponent* MutableThis = const_cast<UMarioMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Mario(*this);
	}
	return ClientPredictionData;
}

#pragma endregion
#pragma region NetworkMoveData

void UMarioMovementComponent::FMarioNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);
	
	const FSavedMove_Mario& CastMove = static_cast<const FSavedMove_Mario&>(ClientMove);
	ThisBounceBoxID = CastMove.SavedThisBounceBoxID;
	OtherBounceBoxID = CastMove.SavedOtherBounceBoxID;
	bBouncedThis = CastMove.bSavedBouncedThis;
	bBouncedOther = CastMove.bSavedBouncedOther;
	bDamagedThis = CastMove.bSavedDamagedThis;
	bDamagedOther = CastMove.bSavedDamagedOther;
}

bool UMarioMovementComponent::FMarioNetworkMoveData::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	const bool bResult = Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);
	SerializeOptionalValue(Ar.IsSaving(), Ar, ThisBounceBoxID, -1);
	SerializeOptionalValue(Ar.IsSaving(), Ar, OtherBounceBoxID, -1);
	Ar << bBouncedThis;
	Ar << bBouncedOther;
	Ar << bDamagedThis;
	Ar << bDamagedOther;

	return bResult;
}

#pragma endregion 
#pragma region Movement

UMarioMovementComponent::UMarioMovementComponent()
{
	SetNetworkMoveDataContainer(MarioMoveDataContainer);
}

void UMarioMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	HitboxManager = GetWorld()->GetSubsystem<UHitboxManager>();
}

void UMarioMovementComponent::MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel)
{
	//This runs on the server, and we copy our network move data into the CMC here to use it during the move.
	const FMarioNetworkMoveData* MoveData = static_cast<const FMarioNetworkMoveData*>(GetCurrentNetworkMoveData());
	ThisHitboxID = MoveData->ThisBounceBoxID;
	OtherHitboxID = MoveData->OtherBounceBoxID;
	bBouncedThis = MoveData->bBouncedThis;
	bBouncedOther = MoveData->bBouncedOther;
	bDamagedThis = MoveData->bDamagedThis;
	bDamagedOther = MoveData->bDamagedOther;
	
	Super::MoveAutonomous(ClientTimeStamp, DeltaTime, CompressedFlags, NewAccel);
}

void UMarioMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	//If we are trying to bounce this frame, we set up the launch before the rest of the movement logic.
	//Launch just sets a flag that is checked during normal movement, so we want it set before that runs for the frame.
	if (bWantsBounce)
	{
		bWantsBounce = false;
		float PingCompensation = 0.0f;
		const APlayerState* PlayerState = GetCharacterOwner()->GetPlayerState();
		if (IsValid(PlayerState))
		{
			PingCompensation = PlayerState->GetPingInMilliseconds() / 1000.0f;
		}
		if (IsValid(HitboxManager) && (GetOwnerRole() != ROLE_Authority || HitboxManager->SanityCheckBounce(ThisHitboxID, OtherHitboxID, PingCompensation)))
		{
			if (bBouncedThis)
			{
				const FVector BounceImpulse = HitboxManager->GetBounceImpulseForHitbox(OtherHitboxID);
				if (BounceImpulse != FVector::ZeroVector)
				{
					Launch(BounceImpulse);
				}
			}
			//If we're authority, then we passed the sanity check above and can apply the authoritative effects of the collision.
			if (GetOwnerRole() == ROLE_Authority && !PawnOwner->IsLocallyControlled())
			{
				HitboxManager->ConfirmCollisionOfHitboxes(ThisHitboxID, OtherHitboxID, bDamagedOther, bBouncedOther);
				HitboxManager->ConfirmCollisionOfHitboxes(OtherHitboxID, ThisHitboxID, bDamagedThis, false);
			}
		}
		ThisHitboxID = -1;
		OtherHitboxID = -1;
		bBouncedThis = false;
		bBouncedOther = false;
		bDamagedThis = false;
		bDamagedOther = false;
	}
}

float UMarioMovementComponent::GetGravityZ() const
{
	if (Velocity.Z > 0.0f)
	{
		return Super::GetGravityZ();
	}
	return Super::GetGravityZ() * DownwardGravityMultiplier;
}

void UMarioMovementComponent::OnHitboxCollision(const int32 ThisID, const int32 OtherID,
	const bool bThisBounced, const bool bOtherBounced, const bool bThisDamaged, const bool bOtherDamaged)
{
	if (!GetPawnOwner()->IsLocallyControlled())
	{
		return;
	}
	bWantsBounce = true;
	ThisHitboxID = ThisID;
	OtherHitboxID = OtherID;
	bBouncedThis = bThisBounced;
	bBouncedOther = bOtherBounced;
	bDamagedThis = bThisDamaged;
	bDamagedOther = bOtherDamaged;
}

#pragma endregion 