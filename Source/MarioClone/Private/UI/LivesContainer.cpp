#include "UI/LivesContainer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBoxSlot.h"

void ULivesContainer::Init(AMarioPlayerCharacter* OwningPlayer)
{
	if (!IsValid(OwningPlayer) || !IsValid(LivesBox))
	{
		return;
	}
	LivesCallback.BindDynamic(this, &ULivesContainer::OnLivesChanged);
	OwningPlayer->SubscribeToLivesChanged(LivesCallback);
	const int32 MaxLives = OwningPlayer->GetMaxLives();
	for (int i = 0; i < MaxLives; i++)
	{
		UImage* LifeImage = WidgetTree->ConstructWidget<UImage>();
		if (IsValid(LifeImage))
		{
			LifeImages.Add(LifeImage);
			UHorizontalBoxSlot* ImageSlot = LivesBox->AddChildToHorizontalBox(LifeImage);
			ImageSlot->SetHorizontalAlignment(HAlign_Fill);
			ImageSlot->SetVerticalAlignment(VAlign_Fill);
			LifeImage->SetBrushFromTexture(LiveTexture);
			LifeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	OnLivesChanged(OwningPlayer->GetCurrentLives());
}

void ULivesContainer::OnLivesChanged(const int32 NewLives)
{
	for (int i = 0; i < LifeImages.Num(); i++)
	{
		UImage* Image = LifeImages[i];
		if (!IsValid(Image))
		{
			continue;
		}
		if (i < NewLives)
		{
			if (Image->GetVisibility() != ESlateVisibility::HitTestInvisible)
			{
				Image->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			if (Image->GetVisibility() != ESlateVisibility::Hidden)
			{
				Image->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}