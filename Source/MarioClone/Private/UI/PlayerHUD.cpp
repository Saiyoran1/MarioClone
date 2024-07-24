#include "UI/PlayerHUD.h"
#include "UI/LivesContainer.h"

void UPlayerHUD::Init(AMarioPlayerCharacter* Player)
{
	if (IsValid(LivesBox))
	{
		LivesBox->Init(Player);
	}

	if (IsValid(ScoreText))
	{
		ScoreCallback.BindDynamic(this, &UPlayerHUD::OnPlayerScoreChanged);
		Player->SubscribeToScoreChanged(ScoreCallback);
		OnPlayerScoreChanged(Player->GetScore());
	}
}

void UPlayerHUD::OnPlayerScoreChanged(const int32 NewScore)
{
	if (IsValid(ScoreText))
	{
		ScoreText->SetText(FText::FromString(FString::Printf(L"%i", NewScore)));
	}
}