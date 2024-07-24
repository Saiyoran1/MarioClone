#include "UI/RespawnWidget.h"
#include "NPCCharacter.h"
#include "Components/ProgressBar.h"

void URespawnWidget::Init(ANPCCharacter* RespawningCharacter)
{
	if (!IsValid(RespawningCharacter))
	{
		return;
	}
	CharacterRef = RespawningCharacter;
	RespawnStartTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	RespawnEndTime = CharacterRef->GetRespawnTime();
	if (IsValid(RespawnProgress))
	{
		RespawnProgress->SetPercent(1.0f);
	}
}

void URespawnWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(CharacterRef))
	{
		return;
	}
	if (IsValid(RespawnProgress))
	{
		const float CurrentTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		const float Percent = (CurrentTime - RespawnStartTime) / FMath::Max(0.01f, (RespawnEndTime - RespawnStartTime));
		RespawnProgress->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}