// ArenaPlayerController.cpp
#include "ArenaPlayerController.h"

void AArenaPlayerController::Client_WaveStart_Implementation(int32 WaveNumber) { BP_OnWaveStart(WaveNumber); }
void AArenaPlayerController::Client_GameWon_Implementation() { BP_OnGameWon(); }
void AArenaPlayerController::Client_GameOver_Implementation() { BP_OnGameOver(); }
