#pragma once
#include "CoreMinimal.h"
#include "MarioPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUD.generated.h"

class UTextBlock;
class ULivesContainer;
class UCanvasPanel;

UCLASS()
class MARIOCLONE_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(AMarioPlayerCharacter* Player);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UCanvasPanel* MainPanel;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	ULivesContainer* LivesBox;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* ScoreText;

	FScoreCallback ScoreCallback;
	UFUNCTION()
	void OnPlayerScoreChanged(const int32 NewScore);
};
