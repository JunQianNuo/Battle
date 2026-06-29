// ArenaPlayerController.h - minimal extension for HUD events
#pragma once
#include "CoreMinimal.h"
#include "battlePlayerController.h"
#include "ArenaPlayerController.generated.h"

UCLASS()
class BATTLE_API AArenaPlayerController : public AbattlePlayerController
{
	GENERATED_BODY()
public:
	UFUNCTION(Client, Reliable)
	void Client_WaveStart(int32 WaveNumber);
	UFUNCTION(Client, Reliable)
	void Client_GameWon();
	UFUNCTION(Client, Reliable)
	void Client_GameOver();
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnKillMessage(const FString& Killer, const FString& Victim);
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnWaveStart(int32 Wave);
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnGameWon();
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnGameOver();
};
