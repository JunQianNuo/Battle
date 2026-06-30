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
#include "AIController.h"
#include "ShooterWeapon.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// Use BP AI controller (has StateTree asset configured)
	static ConstructorHelpers::FClassFinder<AAIController> AICls(TEXT("/Game/Variant_Shooter/Blueprints/AI/BP_ShooterAIController.BP_ShooterAIController_C"));
	if (AICls.Succeeded()) { AIControllerClass = AICls.Class; }
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Use template NPC mesh if available (has proper animations)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	// AnimBP set per-type in SpawnEnemyOfType (melee->Unarmed, ranged->Pistol)

	// Prevent mesh from blocking AI navigation
	GetMesh()->SetCanEverAffectNavigation(false);
	GetCapsuleComponent()->SetCanEverAffectNavigation(true);
}

FVector AEnemyBase::GetWeaponTargetLocation()
{
	// Aim from enemy chest height toward the current target
	FVector AimSource = GetActorLocation() + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	FVector AimTarget;
	if (CurrentAimTarget)
	{
		AimTarget = CurrentAimTarget->GetActorLocation();
		// Offset toward target center (chest)
		AimTarget.Z -= 40.0f;
	}
	else
	{
		// No target: aim forward from source
		AimTarget = AimSource + GetActorForwardVector() * AimRange;
	}

	// Raycast for obstruction
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, AimSource, AimTarget, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return AimTarget;
}

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority() || bIsDead) return;

	// Void death: enemies that fall off the map auto-die to prevent stuck waves
	if (GetActorLocation().Z < -1000.0f)
	{
		Die();
		return;
	}

	UWorld* World = GetWorld();
	const FVector MyLoc = GetActorLocation();

	// Find nearest player (use DistSquared to avoid sqrt)
	APawn* Target = nullptr;
	float NearestSq = FLT_MAX;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* P = PC->GetPawn())
			{
				float DistSq = FVector::DistSquared(MyLoc, P->GetActorLocation());
				if (DistSq < NearestSq) { NearestSq = DistSq; Target = P; }
			}
		}
	}
	if (!Target) return;

	// NavMesh pathfinding (throttled to every 0.5s)
	float Now = World->GetTimeSeconds();
	if (Now - LastPathTime > 0.5f)
	{
		AAIController* AIC = Cast<AAIController>(GetController());
		const float ShootRangeSq = ShootRange * ShootRange;
		if (bIsMelee || NearestSq > ShootRangeSq)
		{
			if (AIC) AIC->MoveToActor(Target, bIsMelee ? MeleeRange * 0.5f : ShootRange);
		}
		else
		{
			if (AIC) AIC->StopMovement(); // ranged: stop and shoot
		}
		LastPathTime = Now;
	}

	// Face target (skip if rotation already matches)
	FVector FaceDir = (Target->GetActorLocation() - MyLoc).GetSafeNormal2D();
	if (!FaceDir.IsNearlyZero())
	{
		FRotator DesiredRot = FaceDir.Rotation();
		if (!DesiredRot.Equals(GetActorRotation(), 0.5f))
			SetActorRotation(DesiredRot);
	}

	// Melee attack in range
	if (bIsMelee && NearestSq < MeleeRange * MeleeRange)
	{
		MeleeAttack(Target);
	}
	// Ranged shooting
	else if (!bIsMelee && Weapon && NearestSq < ShootRange * ShootRange)
	{
		if (!bIsShooting) StartShooting(Target);
	}
	else if (!bIsMelee && bIsShooting)
	{
		StopShooting();
	}
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// Load and cache melee animation sequence (lazy, created as montage on first attack)
	// Moved to DoMeleeAttack

	GetCharacterMovement()->MaxWalkSpeed *= SpeedMultiplier;

	// Slow down enemy fire rate by 50%
	if (Weapon) Weapon->SetRefireRate(1.0f); // double refire interval = half fire rate

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
		World->GetTimerManager().ClearTimer(DmgTimer);
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

	// Play melee animation
	if (UAnimInstance* AI = GetMesh()->GetAnimInstance())
	{
		static UAnimSequence* Seq = Cast<UAnimSequence>(StaticLoadObject(
			UAnimSequence::StaticClass(), nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")));
		if (Seq) AI->PlaySlotAnimationAsDynamicMontage(Seq, FName("DefaultSlot"));
	}

	// Apply damage (delayed to match animation, using weak refs for safety)
	TWeakObjectPtr<AEnemyBase> WeakThis(this);
	TWeakObjectPtr<AActor> WeakTarget(Target);
	GetWorld()->GetTimerManager().SetTimer(DmgTimer, [WeakThis, WeakTarget]()
	{
		if (WeakThis.IsValid() && WeakTarget.IsValid())
		{
			FPointDamageEvent DmgEvt(WeakThis->MeleeDamage, FHitResult(), WeakThis->GetActorForwardVector(), nullptr);
			WeakTarget->TakeDamage(WeakThis->MeleeDamage, DmgEvt, WeakThis->GetController(), WeakThis.Get());
		}
	}, 0.25f, false);

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
