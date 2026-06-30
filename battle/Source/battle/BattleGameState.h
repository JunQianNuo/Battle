// BattleGameState.h - Replicated game state for arena survival
// Tracks waves, scores, enemies, and victory condition

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BattleGameState.generated.h"

/** Types of enemies in the arena */
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Runner  UMETA(DisplayName = "Runner (Fast Melee)"),
	Shooter UMETA(DisplayName = "Shooter (Ranged)"),
	Tank    UMETA(DisplayName = "Tank (Slow Heavy)")
};

/** Per-wave configuration */
USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 EnemyCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float SpawnInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TArray<TSubclassOf<class AEnemyBase>> EnemyTypes;
};

UCLASS()
class BATTLE_API ABattleGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABattleGameState();

	// === Replicated state ===

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 CurrentWave;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 EnemiesRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 TotalScore;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	float RemainingTime = 120.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 TargetScore = 1000;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameWon = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	bool bGameOver = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	TArray<FWaveConfig> WaveConfigs;

	// === Functions ===

	void ReportEnemyKilled(AController* Killer, int32 ScoreValue);
	void ReportEnemySpawned();
	void AddScore(int32 Points);
	void SetWave(int32 WaveNumber, int32 EnemyCount);
	void SetGameWon();
	void SetGameOver();

	// === Blueprint events ===

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnScoreChanged(int32 NewScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnWaveChanged(int32 NewWave, int32 EnemiesInWave);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnGameWon();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnGameOver();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
