// EnemyBase.cpp

#include "EnemyBase.h"
#include "BattleGameState.h"
#include "BattlePlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Variant_Shooter/AI/ShooterAIController.h"
#include "NavigationSystem.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Use BP AI controller (has StateTree asset configured)
	static ConstructorHelpers::FClassFinder<AAIController> AICls(TEXT("/Game/Variant_Shooter/Blueprints/AI/BP_ShooterAIController.BP_ShooterAIController_C"));
	if (AICls.Succeeded()) { AIControllerClass = AICls.Class; }
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Set default skeletal mesh (Mannequin)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// Set default animation blueprint
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	if (AnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBP.Class);
	}

	// Prevent mesh from blocking AI navigation
	GetMesh()->SetCanEverAffectNavigation(false);
	GetCapsuleComponent()->SetCanEverAffectNavigation(true);
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority() || bIsDead) return;

	// Find nearest player
	APawn* Target = nullptr;
	float Nearest = 99999.0f;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* P = PC->GetPawn())
			{
				float Dist = FVector::Dist(GetActorLocation(), P->GetActorLocation());
				if (Dist < Nearest) { Nearest = Dist; Target = P; }
			}
		}
	}
	if (!Target) return;

	// Move toward target
	FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	AddMovementInput(Dir, 1.0f);

	// Attack when in range
	if (bIsMelee && Nearest < MeleeRange)
	{
		MeleeAttack(Target);
	}
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed *= SpeedMultiplier;

	if (bIsMelee)
	{
		OnActorBeginOverlap.AddDynamic(this, &AEnemyBase::OnEnemyOverlap);
	}

	// Force-start StateTree and configure perception
	if (AShooterAIController* AIC = Cast<AShooterAIController>(GetController()))
	{
		// Start StateTree
		if (UStateTreeAIComponent* ST = AIC->FindComponentByClass<UStateTreeAIComponent>())
		{
			ST->StartLogic();
		}

		// Ensure sight perception is configured (some BP setups may not have it)
		if (UAIPerceptionComponent* Perc = AIC->FindComponentByClass<UAIPerceptionComponent>())
		{
			// Add sight sense config
			UAISenseConfig_Sight* Sight = NewObject<UAISenseConfig_Sight>(Perc);
			Sight->SightRadius = 3000.0f;
			Sight->LoseSightRadius = 3500.0f;
			Sight->PeripheralVisionAngleDegrees = 90.0f;
			Sight->DetectionByAffiliation.bDetectEnemies = true;
			Sight->DetectionByAffiliation.bDetectNeutrals = true;
			Sight->DetectionByAffiliation.bDetectFriendlies = true;
			Perc->ConfigureSense(*Sight);
			Perc->RequestStimuliListenerUpdate();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Enemy] Speed=%.0f Melee=%d HP=%.0f Controller=%s"),
		GetCharacterMovement()->MaxWalkSpeed, (int32)bIsMelee, CurrentHP,
		GetController() ? *GetController()->GetClass()->GetName() : TEXT("NONE"));
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MeleeTimer);
	}
}

float AEnemyBase::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Track who last damaged us for kill credit
	if (EventInstigator)
	{
		LastDamager = EventInstigator;
	}
	return Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void AEnemyBase::Die()
{
	// Report kill to GameState with the correct killer
	if (HasAuthority())
	{
		if (ABattleGameState* GS = GetWorld()->GetGameState<ABattleGameState>())
		{
			GS->ReportEnemyKilled(LastDamager.Get(), ScoreValue);
		}
	}

	Super::Die();
}

void AEnemyBase::MeleeAttack(AActor* Target)
{
	if (!bMeleeReady || !Target) return;
	if (!HasAuthority()) return;

	DoMeleeAttack(Target);
}

void AEnemyBase::DoMeleeAttack(AActor* Target)
{
	bMeleeReady = false;
	LastMeleeTime = GetWorld()->GetTimeSeconds();

	// Apply damage to target
	FPointDamageEvent DamageEvent(MeleeDamage, FHitResult(), GetActorForwardVector(), nullptr);
	Target->TakeDamage(MeleeDamage, DamageEvent, GetController(), this);

	// Apply knockback
	if (ACharacter* TargetChar = Cast<ACharacter>(Target))
	{
		FVector KnockbackDir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		KnockbackDir.Z = 0.3f; // slight upward
		TargetChar->GetCharacterMovement()->AddImpulse(KnockbackDir * MeleeKnockback, true);
	}

	// Start cooldown
	GetWorld()->GetTimerManager().SetTimer(
		MeleeTimer, this, &AEnemyBase::ResetMeleeCooldown, MeleeCooldown, false);
}

void AEnemyBase::ResetMeleeCooldown()
{
	bMeleeReady = true;
}

void AEnemyBase::OnEnemyOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// Only deal contact damage to players
	if (!bIsMelee || !bMeleeReady || !HasAuthority()) return;
	if (!OtherActor || OtherActor == this) return;

	// Check if it's a player character
	ACharacter* OtherChar = Cast<ACharacter>(OtherActor);
	if (!OtherChar) return;

	// Don't damage other enemies
	if (OtherChar->IsA<AShooterNPC>()) return;

	DoMeleeAttack(OtherChar);
}
