#include "UI/GameOverScreen.h"

void UGameOverScreen::Init(const bool bGameWon, const FRestartCallback& Callback)
{
	if (Callback.IsBound())
	{
		RestartCallback = Callback;
	}
	if (IsValid(GameOverText))
	{
		const FString GameOverString = bGameWon ? "You won! :)" : "You lost! :(";
		GameOverText->SetText(FText::FromString(GameOverString));
	}
	if (IsValid(RestartButton))
	{
		RestartButton->OnClicked.AddUnique(RestartCallback);
	}
}