// ArenaGameMode.h - Arena survival: survive 2 minutes, enemies respawn every 20s

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	// === Survival Config ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	float SurvivalDuration = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	float SpawnInterval = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	int32 EnemiesPerSpawn = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Survival")
	int32 MaxPlayers = 4;

	// Menu state: true while waiting for player to start
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bInMenu = true;

	// === Game flow ===

	UFUNCTION(BlueprintCallable, Category = "Game")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void HostGame(const FString& MapName);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void JoinGame(const FString& ServerIP);

	// Console commands: press ~ and type "host" or "join 192.168.x.x"
	UFUNCTION(Exec)
	void Host(FString MapName = TEXT("Lvl_Shooter")) { HostGame(MapName); }

	UFUNCTION(Exec)
	void Join(FString IP = TEXT("127.0.0.1")) { JoinGame(IP); }

protected:
	FTimerHandle SpawnTimer;
	FTimerHandle SurvivalTimer;
	FTimerHandle StartGameTimer;
	TArray<TWeakObjectPtr<AEnemyBase>> ActiveEnemies;
	TArray<AActor*> EnemySpawnPoints;
	int32 ConnectedPlayers = 0;
	TSet<APlayerController*> SpawnedPlayers;
	bool LastPawnWasAlive = true;
	float ElapsedTime = 0.0f;
	int32 SpawnCount = 0;

	void SpawnWeaponPickups();
	void NotifyDeathPenalty(int32 Points);
public:
	void DoPlayerMelee(); // public for mobile HUD call
protected:
	void SpawnEnemyBatch();
	void SpawnEnemyOfType(TSubclassOf<AEnemyBase> EnemyClass, const FVector& SpawnLocation);

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnGameStart();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game")
	void BP_OnGameWon();

public:
	UFUNCTION(BlueprintPure, Category = "Game")
	int32 GetConnectedPlayers() const { return ConnectedPlayers; }
};
