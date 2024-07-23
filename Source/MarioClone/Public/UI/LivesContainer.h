#pragma once
#include "CoreMinimal.h"
#include "MarioPlayerCharacter.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "LivesContainer.generated.h"

UCLASS()
class MARIOCLONE_API ULivesContainer : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(AMarioPlayerCharacter* OwningPlayer);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UHorizontalBox* LivesBox;
	UPROPERTY(EditAnywhere, Category = "Lives")
	UTexture2D* LiveTexture;

	UPROPERTY()
	TArray<UImage*> LifeImages;
	FLivesCallback LivesCallback;
	UFUNCTION()
	void OnLivesChanged(const int32 NewLives);
};
