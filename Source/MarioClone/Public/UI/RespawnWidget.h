#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RespawnWidget.generated.h"

class ANPCCharacter;
class UImage;
class UProgressBar;

UCLASS()
class MARIOCLONE_API URespawnWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void Init(ANPCCharacter* RespawningCharacter);

private:

	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UProgressBar* RespawnProgress;
	UPROPERTY(VisibleAnywhere, meta = (BindWidget))
	UImage* RespawnImage;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	ANPCCharacter* CharacterRef;
	float RespawnStartTime = 0.0f;
	float RespawnEndTime = 0.0f;
};
