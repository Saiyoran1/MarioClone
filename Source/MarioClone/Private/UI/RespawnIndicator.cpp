#include "UI/RespawnIndicator.h"

#include "NPCCharacter.h"
#include "Components/WidgetComponent.h"
#include "UI/RespawnWidget.h"

ARespawnIndicator::ARespawnIndicator()
{
	PrimaryActorTick.bCanEverTick = false;

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(FName(TEXT("Widget")));
	SetRootComponent(WidgetComponent);
}

void ARespawnIndicator::Init(ANPCCharacter* RespawningCharacter)
{
	if (!IsValid(RespawningCharacter) || !IsValid(WidgetClass))
	{
		return;
	}
	URespawnWidget* RespawnWidget = CreateWidget<URespawnWidget>(GetWorld(), WidgetClass);
	if (IsValid(RespawnWidget))
	{
		WidgetComponent->SetWidget(RespawnWidget);
		RespawnWidget->Init(RespawningCharacter);
	}
}
