#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUD.generated.h"

class ULivesContainer;
class AMarioPlayerCharacter;
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
};
