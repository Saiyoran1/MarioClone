#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameOverScreen.generated.h"

DECLARE_DYNAMIC_DELEGATE(FRestartCallback);

UCLASS()
class MARIOCLONE_API UGameOverScreen : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(const bool bGameWon, const FRestartCallback& Callback);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UTextBlock* GameOverText;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UButton* RestartButton;

	FRestartCallback RestartCallback;
};
