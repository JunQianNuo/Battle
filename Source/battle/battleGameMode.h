// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "battleGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AbattleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AbattleGameMode();
};



