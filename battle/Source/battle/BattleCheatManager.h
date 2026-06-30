// BattleCheatManager.h - Console commands for multiplayer
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "BattleCheatManager.generated.h"

UCLASS()
class BATTLE_API UBattleCheatManager : public UCheatManager
{
	GENERATED_BODY()

	UFUNCTION(Exec)
	void Host(FString MapName = TEXT("Lvl_Shooter"));

	UFUNCTION(Exec)
	void Join(FString IP = TEXT("127.0.0.1"));
};
