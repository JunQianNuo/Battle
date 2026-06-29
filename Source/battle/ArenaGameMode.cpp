// ArenaGameMode.cpp

#include "ArenaGameMode.h"
#include "EnemyBase.h"
#include "BattleGameState.h"
#include "BattlePlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ShooterWeaponHolder.h"
#include "ShooterWeapon.h"
#include "battleCharacter.h"
#include "Camera/CameraComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

AArenaGameMode::AArenaGameMode()
{
	GameStateClass = ABattleGameState::StaticClass();
	PlayerStateClass = ABattlePlayerState::StaticClass();
	// Use BP subclass (C++ class is abstract)
	static ConstructorHelpers::FClassFinder<APlayerController> PCClass(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterPlayerController.BP_ShooterPlayerController_C"));
	if (PCClass.Succeeded()) { PlayerControllerClass = PCClass.Class; }

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterCharacter.BP_ShooterCharacter_C"));
	if (PawnClass.Succeeded()) { DefaultPawnClass = PawnClass.Class; }

	// Default waves - will be overridden by BP CDO if configured
	if (Waves.Num() == 0)
	{
		FWaveConfig W1;
		W1.EnemyCount = 4;  W1.SpawnInterval = 2.5f;
		W1.EnemyTypes.Add(AEnemyBase::StaticClass());
		Waves.Add(W1);

		FWaveConfig W2;
		W2.EnemyCount = 6;  W2.SpawnInterval = 2.0f;
		W2.EnemyTypes.Add(AEnemyBase::StaticClass());
		Waves.Add(W2);

		FWaveConfig W3;
		W3.EnemyCount = 8;  W3.SpawnInterval = 1.5f;
		W3.EnemyTypes.Add(AEnemyBase::StaticClass());
		Waves.Add(W3);

		FWaveConfig W4;
		W4.EnemyCount = 10; W4.SpawnInterval = 1.2f;
		W4.EnemyTypes.Add(AEnemyBase::StaticClass());
		Waves.Add(W4);

		FWaveConfig W5;
		W5.EnemyCount = 12; W5.SpawnInterval = 1.0f;
		W5.EnemyTypes.Add(AEnemyBase::StaticClass());
		Waves.Add(W5);
	}
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Cache spawn points via tag
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), SpawnPointTag, EnemySpawnPoints);
	if (EnemySpawnPoints.Num() == 0)
	{
		// Fallback: use all PlayerStarts as enemy spawn points
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), EnemySpawnPoints);
	}

	// Initialize game state
	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		GS->TargetScore = VictoryScore;
		GS->WaveConfigs = Waves;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		UE_LOG(LogTemp, Log, TEXT("[ArenaGM] NavSys ready"));
	}

	SpawnWeaponPickups();
	BP_OnGameStart();
}

void AArenaGameMode::SpawnWeaponPickups()
{
	UClass* PickupClass = StaticLoadClass(AActor::StaticClass(), nullptr,
		TEXT("/Game/Variant_Shooter/Blueprints/Pickups/BP_ShooterPickup.BP_ShooterPickup_C"));
	if (!PickupClass) return;

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	for (AActor* PS : PlayerStarts)
	{
		FVector Loc = PS->GetActorLocation() + FVector(200.0f, 0.0f, 50.0f);
		GetWorld()->SpawnActor<AActor>(PickupClass, Loc, FRotator::ZeroRotator);
		UE_LOG(LogTemp, Log, TEXT("[ArenaGM] Weapon pickup spawned at %s"), *Loc.ToString());
	}
}

void AArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ConnectedPlayers++;

	if (ConnectedPlayers == 1)
	{
		GetWorld()->GetTimerManager().SetTimer(StartGameTimer, this, &AArenaGameMode::StartGame, 2.0f, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Player joined. Total: %d"), ConnectedPlayers);
}

void AArenaGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	ConnectedPlayers = FMath::Max(0, ConnectedPlayers - 1);
	if (ConnectedPlayers == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(StartGameTimer);
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
		GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
		bWaveInProgress = false;
	}
	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Player left. Total: %d"), ConnectedPlayers);
}

void AArenaGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("[ArenaGM] Player spawned"));
}

void AArenaGameMode::StartGame()
{
	if (Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[ArenaGameMode] Cannot start: no waves configured!"));
		return;
	}
	CurrentWaveIndex = 0;
	StartNextWave();

	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		GS->TargetScore = VictoryScore;
		GS->WaveConfigs = Waves; // Sync to replicated GameState for client UI
		GS->SetWave(1, Waves[0].EnemyCount);
	}
}

void AArenaGameMode::RestartGame()
{
	// Clean up remaining enemies
	for (AEnemyBase* Enemy : ActiveEnemies)
	{
		if (Enemy) Enemy->Destroy();
	}
	ActiveEnemies.Empty();

	GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	// Reset game state
	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		GS->TotalScore = 0;
		GS->CurrentWave = 0;
		GS->EnemiesRemaining = 0;
		GS->bGameWon = false;
		GS->bGameOver = false;
	}

	StartGame();
}

void AArenaGameMode::StartNextWave()
{
	if (CurrentWaveIndex >= Waves.Num())
	{
		// Loop back with increased difficulty: reuse last wave config with more enemies
		CurrentWaveIndex = Waves.Num() - 1;
	}

	if (Waves.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[ArenaGameMode] No waves configured!"));
		return;
	}
	bWaveInProgress = true;

	const FWaveConfig& WaveConfig = Waves[CurrentWaveIndex];
	EnemiesSpawnedThisWave = 0;

	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Starting Wave %d with %d enemies"), CurrentWaveIndex + 1, WaveConfig.EnemyCount);

	BP_OnWaveStart(CurrentWaveIndex + 1);

	// Spawn enemies with interval
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, [this]()
	{
		SpawnEnemy();
	}, WaveConfig.SpawnInterval, true, 0.0f);
}

void AArenaGameMode::SpawnEnemy()
{
	if (CurrentWaveIndex >= Waves.Num()) return;

	const FWaveConfig& WaveConfig = Waves[CurrentWaveIndex];

	if (EnemiesSpawnedThisWave >= WaveConfig.EnemyCount)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
		return;
	}

	// Pick a random spawn point
	FVector SpawnLoc = FVector::ZeroVector;
	if (EnemySpawnPoints.Num() > 0)
	{
		int32 Idx = FMath::RandRange(0, EnemySpawnPoints.Num() - 1);
		if (EnemySpawnPoints[Idx])
		{
			SpawnLoc = EnemySpawnPoints[Idx]->GetActorLocation();
		}
	}

	// Pick enemy type from wave config (random, or first available)
	TSubclassOf<AEnemyBase> EnemyClass = nullptr;
	if (WaveConfig.EnemyTypes.Num() > 0)
	{
		EnemyClass = WaveConfig.EnemyTypes[FMath::RandRange(0, WaveConfig.EnemyTypes.Num() - 1)];
	}

	if (EnemyClass)
	{
		SpawnEnemyOfType(EnemyClass, SpawnLoc);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArenaGameMode] No enemy types configured for wave %d!"), CurrentWaveIndex + 1);
	}

	EnemiesSpawnedThisWave++;
}

void AArenaGameMode::SpawnEnemyOfType(TSubclassOf<AEnemyBase> EnemyClass, const FVector& SpawnLocation)
{
	// Use deferred spawn to set randomized properties BEFORE BeginPlay runs
	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
	AEnemyBase* Enemy = GetWorld()->SpawnActorDeferred<AEnemyBase>(EnemyClass, SpawnTransform);
	if (!Enemy) return;

	// Randomize enemy type BEFORE BeginPlay (fixes speed/melee initialization)
	int32 R = FMath::RandRange(0, 2);
	if (R == 0)
	{
		Enemy->EnemyType = EEnemyType::Runner;
		Enemy->ScoreValue = 100;
		Enemy->SpeedMultiplier = 1.5f;
		Enemy->bIsMelee = true;
		Enemy->MeleeDamage = 15.0f;
		Enemy->CurrentHP = 50.0f;
	}
	else if (R == 1)
	{
		Enemy->EnemyType = EEnemyType::Shooter;
		Enemy->ScoreValue = 150;
		Enemy->SpeedMultiplier = 0.8f;
		Enemy->bIsMelee = false;
		Enemy->CurrentHP = 100.0f;
	}
	else
	{
		Enemy->EnemyType = EEnemyType::Tank;
		Enemy->ScoreValue = 300;
		Enemy->SpeedMultiplier = 0.5f;
		Enemy->bIsMelee = true;
		Enemy->MeleeDamage = 40.0f;
		Enemy->CurrentHP = 300.0f;
	}

	Enemy->FinishSpawning(SpawnTransform);

	Enemy->OnPawnDeath.AddDynamic(this, &AArenaGameMode::OnEnemyKilled);
	ActiveEnemies.Add(Enemy);

	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		GS->ReportEnemySpawned();
	}
}

void AArenaGameMode::OnEnemyKilled()
{
	CheckWaveComplete();
}

void AArenaGameMode::CheckWaveComplete()
{
	if (!bWaveInProgress) return;
	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		if (GS->EnemiesRemaining <= 0)
		{
			EndWave();
		}
		if (GS->bGameWon)
		{
			GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
			GetWorld()->GetTimerManager().ClearTimer(WaveTimer);
			bWaveInProgress = false;
		}
	}
}

void AArenaGameMode::EndWave()
{
	bWaveInProgress = false;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	BP_OnWaveComplete(CurrentWaveIndex + 1);

	// Clean dead enemies from active list
	ActiveEnemies.RemoveAll([](AEnemyBase* E) { return !IsValid(E); });

	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Wave %d complete!"), CurrentWaveIndex + 1);

	CurrentWaveIndex++;

	// Wait before next wave
	GetWorld()->GetTimerManager().SetTimer(WaveTimer, this, &AArenaGameMode::StartNextWave, TimeBetweenWaves, false);
}

void AArenaGameMode::HostGame(const FString& MapName)
{
	FString Command = MapName + TEXT("?listen");
	GetWorld()->ServerTravel(Command);
}

void AArenaGameMode::JoinGame(const FString& ServerIP)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		PC->ClientTravel(ServerIP, ETravelType::TRAVEL_Absolute);
	}
}
