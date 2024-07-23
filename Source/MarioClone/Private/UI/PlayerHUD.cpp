#include "UI/PlayerHUD.h"
#include "UI/LivesContainer.h"

void UPlayerHUD::Init(AMarioPlayerCharacter* Player)
{
	if (IsValid(LivesBox))
	{
		LivesBox->Init(Player);
	}
}