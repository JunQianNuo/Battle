// ArenaGameMode.h - Arena survival wave system + multiplayer (inherits ShooterGameMode for proper player spawning)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BattleGameState.h"
#include "ArenaGameMode.generated.h"

class AEnemyBase;

UCLASS()
class BATTLE_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// === Wave Configuration ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	TArray<FWaveConfig> Waves;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	float TimeBetweenWaves = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	int32 VictoryScore = 1500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multiplayer")
	int32 MaxPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waves")
	FName SpawnPointTag = FName("EnemySpawn");

	// === Game flow ===

	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void HostGame(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void JoinGame(const FString& ServerIP);

protected:
	int32 CurrentWaveIndex = 0;
	int32 EnemiesSpawnedThisWave = 0;
	FTimerHandle WaveTimer;
	FTimerHandle SpawnTimer;
	TArray<AEnemyBase*> ActiveEnemies;
	TArray<AActor*> EnemySpawnPoints;
	int32 ConnectedPlayers = 0;
	FTimerHandle StartGameTimer;
	bool bWaveInProgress = false;

	void SpawnWeaponPickups();
	void StartNextWave();
	void EndWave();
	void SpawnEnemy();
	void SpawnEnemyOfType(TSubclassOf<AEnemyBase> EnemyClass, const FVector& SpawnLocation);
	UFUNCTION()
	void OnEnemyKilled();
	void CheckWaveComplete();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnWaveStart(int32 WaveNumber);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnWaveComplete(int32 WaveNumber);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnGameStart();

public:
	UFUNCTION(BlueprintPure, Category = "Game")
	int32 GetConnectedPlayers() const { return ConnectedPlayers; }
};
