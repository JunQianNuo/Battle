// ArenaGameMode.cpp — survival mode: 2 minutes, enemies every 20s

#include "ArenaGameMode.h"
#include "EnemyBase.h"
#include "BattleGameState.h"
#include "ArenaPlayerController.h"
#include "battleCharacter.h"
#include "Animation/AnimSequence.h"
#include "Engine/DamageEvents.h"
#include "BattlePlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ShooterWeaponHolder.h"
#include "ShooterWeapon.h"
#include "Camera/CameraComponent.h"
#include "BattlHUD.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"

AArenaGameMode::AArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = ABattleGameState::StaticClass();
	PlayerStateClass = ABattlePlayerState::StaticClass();
	HUDClass = ABattlHUD::StaticClass();
	// Use BP subclass (C++ class is abstract)
	static ConstructorHelpers::FClassFinder<APlayerController> PCBP(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterPlayerController.BP_ShooterPlayerController_C"));
	if (PCBP.Succeeded()) { PlayerControllerClass = PCBP.Class; }

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterCharacter.BP_ShooterCharacter_C"));
	if (PawnClass.Succeeded()) { DefaultPawnClass = PawnClass.Class; }
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Cache spawn points
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("EnemySpawn"), EnemySpawnPoints);
	if (EnemySpawnPoints.Num() == 0)
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), EnemySpawnPoints);
	UE_LOG(LogTemp, Log, TEXT("[Arena] %d spawn points found"), EnemySpawnPoints.Num());

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
	}
}

void AArenaGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(StartGameTimer);
		World->GetTimerManager().ClearTimer(SpawnTimer);
		World->GetTimerManager().ClearTimer(SurvivalTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ConnectedPlayers++;

	// Don't auto-start — player must click START on the menu
	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Player joined. Total: %d (menu)"), ConnectedPlayers);
}

void AArenaGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	ConnectedPlayers = FMath::Max(0, ConnectedPlayers - 1);
	if (ConnectedPlayers == 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(StartGameTimer);
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
		GetWorld()->GetTimerManager().ClearTimer(SurvivalTimer);
	}
	UE_LOG(LogTemp, Log, TEXT("[ArenaGameMode] Player left. Total: %d"), ConnectedPlayers);
}

void AArenaGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
		SpawnedPlayers.Add(PC);
}

void AArenaGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->WasInputKeyJustPressed(EKeys::F))
			DoPlayerMelee();

		// Detect death: pawn went from alive to tagged Dead
		if (APawn* P = PC->GetPawn())
		{
			static const FName DeadTag("Dead");
			bool bAlive = !P->ActorHasTag(DeadTag);
			if (!bAlive && LastPawnWasAlive && SpawnedPlayers.Contains(PC))
			{
				if (ABattlePlayerState* PS = PC->GetPlayerState<ABattlePlayerState>())
				{
					PS->AddDeath();
					PS->AddScore(-500);
				}
				if (ABattleGameState* GS = GetGameState<ABattleGameState>())
					GS->AddScore(-500);
				NotifyDeathPenalty(500);
			}
			LastPawnWasAlive = bAlive;
		}
	}

	// Update countdown timer on GameState
	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		if (!GS->bGameWon)
		{
			ElapsedTime += DeltaTime;
			GS->RemainingTime = FMath::Max(0.0f, SurvivalDuration - ElapsedTime);
		}
	}
}

void AArenaGameMode::NotifyDeathPenalty(int32 Points)
{
	GEngine->AddOnScreenDebugMessage(3, 3.0f, FColor::Red,
		FString::Printf(TEXT("-%d"), Points));
}

void AArenaGameMode::DoPlayerMelee()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;
	APawn* P = PC->GetPawn();
	if (!P) return;

	FVector Start = P->GetActorLocation();
	FVector End = Start + P->GetActorForwardVector() * 250.0f;
	AbattleCharacter* BC = Cast<AbattleCharacter>(P);
	if (BC)
	{
		Start = BC->GetFirstPersonCameraComponent()->GetComponentLocation();
		End = Start + BC->GetFirstPersonCameraComponent()->GetForwardVector() * 250.0f;
	}

	FHitResult Hit;
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(P);

	// Play melee animation on first-person and third-person meshes
	static UAnimSequence* MeleeSeq = Cast<UAnimSequence>(StaticLoadObject(
		UAnimSequence::StaticClass(), nullptr,
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")));
	if (MeleeSeq)
	{
		static const FName DefaultSlot("DefaultSlot");
		if (BC)
		{
			if (USkeletalMeshComponent* FP = BC->GetFirstPersonMesh())
			{
				if (UAnimInstance* AI = FP->GetAnimInstance())
					AI->PlaySlotAnimationAsDynamicMontage(MeleeSeq, DefaultSlot);
			}
		}
		if (ACharacter* C = Cast<ACharacter>(P))
		{
			if (UAnimInstance* AI = C->GetMesh()->GetAnimInstance())
				AI->PlaySlotAnimationAsDynamicMontage(MeleeSeq, DefaultSlot);
		}
	}

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(60.0f), Q))
	{
		if (ACharacter* V = Cast<ACharacter>(Hit.GetActor()))
		{
			FPointDamageEvent Dmg(25.0f, Hit, (End - Start).GetSafeNormal(), nullptr);
			V->TakeDamage(25.0f, Dmg, PC, P);
		}
	}
}

void AArenaGameMode::StartGame()
{
	bInMenu = false;
	ElapsedTime = 0.0f;
	SpawnCount = 0;

	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
	{
		GS->RemainingTime = SurvivalDuration;
		GS->TotalScore = 0;
		GS->bGameWon = false;
		GS->bGameOver = false;
		GS->EnemiesRemaining = 0;
	}

	// Initial enemy batch
	SpawnEnemyBatch();

	// Repeat spawn every SpawnInterval seconds
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AArenaGameMode::SpawnEnemyBatch, SpawnInterval, true, SpawnInterval);

	// Survival victory timer
	GetWorld()->GetTimerManager().SetTimer(SurvivalTimer, [this]()
	{
		if (ABattleGameState* GS = GetGameState<ABattleGameState>())
			GS->SetGameWon();
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	}, SurvivalDuration, false);

	// Release mouse from menu
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	UE_LOG(LogTemp, Log, TEXT("[Arena] Survival started: %.0fs, enemies every %.0fs"), SurvivalDuration, SpawnInterval);
}

void AArenaGameMode::RestartGame()
{
	// Clean up remaining enemies
	for (auto& WeakEnemy : ActiveEnemies)
	{
		if (WeakEnemy.IsValid()) WeakEnemy->Destroy();
	}
	ActiveEnemies.Empty();

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(SurvivalTimer);

	StartGame();
}

void AArenaGameMode::SpawnEnemyBatch()
{
	SpawnCount++;
	int32 Count = EnemiesPerSpawn + (SpawnCount - 1); // 4, 5, 6, 7, 8...

	UE_LOG(LogTemp, Log, TEXT("[Arena] Spawn batch #%d: %d enemies at %.0fs"), SpawnCount, Count, ElapsedTime);

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const FVector PlayerLoc = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	int32 Skipped = 0;

	for (int32 i = 0; i < Count; ++i)
	{
		FVector Loc;
		bool bFound = false;

		if (NavSys && !PlayerLoc.IsZero())
		{
			for (int32 Attempt = 0; Attempt < 15; ++Attempt)
			{
				FVector Offset = FVector(
					FMath::RandRange(-2500.0f, 2500.0f),
					FMath::RandRange(-2500.0f, 2500.0f), 0);
				float Dist = Offset.Size();
				if (Dist < 1000.0f) Offset = Offset.GetSafeNormal() * 1000.0f;
				Offset += PlayerLoc;

				FNavLocation NavLoc;
				if (NavSys->ProjectPointToNavigation(Offset, NavLoc, FVector(2000, 2000, 500)))
				{
					Loc = NavLoc.Location;
					bFound = true;
					break;
				}
			}
		}

		if (!bFound)
		{
			if (EnemySpawnPoints.Num() > 0)
				Loc = EnemySpawnPoints[i % EnemySpawnPoints.Num()]->GetActorLocation();
			else
				Skipped++;
		}

		if (!bFound && Skipped > 0) continue;
		TSubclassOf<AEnemyBase> Cls(AEnemyBase::StaticClass());
		SpawnEnemyOfType(Cls, Loc);
	}

	if (Skipped > 0)
		UE_LOG(LogTemp, Warning, TEXT("[Arena] Batch #%d: %d skipped"), SpawnCount, Skipped);
}

void AArenaGameMode::SpawnEnemyOfType(TSubclassOf<AEnemyBase> EnemyClass, const FVector& SpawnLocation)
{
	static UClass* AnimUnarmed = StaticLoadClass(UAnimInstance::StaticClass(), nullptr,
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	static UClass* AnimPistol = StaticLoadClass(UAnimInstance::StaticClass(), nullptr,
		TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Pistol.ABP_TP_Pistol_C"));

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
	AEnemyBase* Enemy = GetWorld()->SpawnActorDeferred<AEnemyBase>(EnemyClass, SpawnTransform);
	if (!Enemy) return;

	int32 R = FMath::RandRange(0, 2);
	if (R == 0)
	{
		Enemy->EnemyType = EEnemyType::Runner;
		Enemy->ScoreValue = 100;
		Enemy->SpeedMultiplier = 1.0f;
		Enemy->bIsMelee = true;
		Enemy->MeleeDamage = 15.0f;
		Enemy->CurrentHP = 50.0f;
		if (AnimUnarmed) Enemy->GetMesh()->SetAnimInstanceClass(AnimUnarmed);
	}
	else if (R == 1)
	{
		Enemy->EnemyType = EEnemyType::Shooter;
		Enemy->ScoreValue = 150;
		Enemy->SpeedMultiplier = 0.6f;
		Enemy->bIsMelee = false;
		Enemy->CurrentHP = 100.0f;
		if (AnimPistol) Enemy->GetMesh()->SetAnimInstanceClass(AnimPistol);
		static UClass* WC = StaticLoadClass(AShooterWeapon::StaticClass(), nullptr,
			TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Weapons/BP_ShooterWeapon_Pistol.BP_ShooterWeapon_Pistol_C"));
		if (WC) Enemy->SetWeaponClass(WC);
	}
	else
	{
		Enemy->EnemyType = EEnemyType::Tank;
		Enemy->ScoreValue = 300;
		Enemy->SpeedMultiplier = 0.5f;
		Enemy->bIsMelee = true;
		Enemy->MeleeDamage = 40.0f;
		Enemy->CurrentHP = 300.0f;
		if (AnimUnarmed) Enemy->GetMesh()->SetAnimInstanceClass(AnimUnarmed);
	}

	Enemy->FinishSpawning(SpawnTransform);

	Enemy->OnDestroyed.AddDynamic(this, &AArenaGameMode::OnEnemyDestroyed);
	ActiveEnemies.Add(Enemy);

	if (ABattleGameState* GS = GetGameState<ABattleGameState>())
		GS->ReportEnemySpawned();
}

void AArenaGameMode::OnEnemyDestroyed(AActor* DestroyedActor)
{
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(DestroyedActor))
		ActiveEnemies.Remove(Enemy);
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
