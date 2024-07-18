#include "MarioClone/Public/MarioGameMode.h"
#include "MarioClone/Public/MarioPlayerCharacter.h"

AMarioGameMode::AMarioGameMode()
{
	DefaultPawnClass = AMarioPlayerCharacter::StaticClass();
	GameStateClass = AMarioPlayerCharacter::StaticClass();
}