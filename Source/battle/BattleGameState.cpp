// BattleGameState.cpp

#include "BattleGameState.h"
#include "Net/UnrealNetwork.h"
#include "BattlePlayerState.h"

ABattleGameState::ABattleGameState()
{
	CurrentWave = 0;
	EnemiesRemaining = 0;
	TotalScore = 0;
}

void ABattleGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABattleGameState, CurrentWave);
	DOREPLIFETIME(ABattleGameState, EnemiesRemaining);
	DOREPLIFETIME(ABattleGameState, TotalScore);
	DOREPLIFETIME(ABattleGameState, TargetScore);
	DOREPLIFETIME(ABattleGameState, bGameWon);
	DOREPLIFETIME(ABattleGameState, bGameOver);
	DOREPLIFETIME(ABattleGameState, WaveConfigs);
}

void ABattleGameState::ReportEnemyKilled(AController* Killer, int32 ScoreValue)
{
	if (!HasAuthority()) return;

	EnemiesRemaining = FMath::Max(0, EnemiesRemaining - 1);
	AddScore(ScoreValue);

	// Award score to the killing player
	if (Killer)
	{
		if (ABattlePlayerState* PS = Killer->GetPlayerState<ABattlePlayerState>())
		{
			PS->AddScore(ScoreValue);
			PS->AddKill();
		}
	}

	BP_OnScoreChanged(TotalScore);

	// Check victory
	if (TotalScore >= TargetScore && !bGameWon)
	{
		SetGameWon();
	}
}

void ABattleGameState::ReportEnemySpawned()
{
	if (!HasAuthority()) return;
	EnemiesRemaining++;
}

void ABattleGameState::AddScore(int32 Points)
{
	if (!HasAuthority()) return;
	TotalScore += Points;
}

void ABattleGameState::SetWave(int32 WaveNumber, int32 EnemyCount)
{
	if (!HasAuthority()) return;
	CurrentWave = WaveNumber;
	EnemiesRemaining = EnemyCount;
	BP_OnWaveChanged(CurrentWave, EnemyCount);
}

void ABattleGameState::SetGameWon()
{
	if (!HasAuthority()) return;
	bGameWon = true;
	bGameOver = true;
	BP_OnGameWon();
}

void ABattleGameState::SetGameOver()
{
	if (!HasAuthority()) return;
	bGameOver = true;
	BP_OnGameOver();
}
