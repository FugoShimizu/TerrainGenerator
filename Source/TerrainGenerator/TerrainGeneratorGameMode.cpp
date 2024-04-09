// Copyright Epic Games, Inc. All Rights Reserved.

#include "TerrainGeneratorGameMode.h"
#include "TerrainGeneratorCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATerrainGeneratorGameMode::ATerrainGeneratorGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
