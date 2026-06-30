// ArenaPlayerController.cpp
#include "ArenaPlayerController.h"
#include "GameFramework/Character.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "battleCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UAnimSequence* AArenaPlayerController::MeleeAnim = nullptr;

AArenaPlayerController::AArenaPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UAnimSequence> MA(
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	if (MA.Succeeded()) MeleeAnim = MA.Object;
}

void AArenaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Melee bound via Enhanced Input in Blueprint - see BP_ArenaPlayerController
}

void AArenaPlayerController::DoMelee()
{
	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastMeleeTime < MeleeCooldown) return;
	LastMeleeTime = Now;

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	FVector Start = MyPawn->GetActorLocation();
	FVector End = Start + MyPawn->GetActorForwardVector() * MeleeRange;

	if (AbattleCharacter* BC = Cast<AbattleCharacter>(MyPawn))
	{
		Start = BC->GetFirstPersonCameraComponent()->GetComponentLocation();
		End = Start + BC->GetFirstPersonCameraComponent()->GetForwardVector() * MeleeRange;
	}

	FHitResult Hit;
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(MyPawn);
	Q.AddIgnoredActor(this);

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(50.0f), Q))
	{
		if (ACharacter* Victim = Cast<ACharacter>(Hit.GetActor()))
		{
			FPointDamageEvent DmgEvt(MeleeDamage, Hit, (End - Start).GetSafeNormal(), nullptr);
			Victim->TakeDamage(MeleeDamage, DmgEvt, this, MyPawn);
		}
	}

	if (ACharacter* C = Cast<ACharacter>(MyPawn))
	{
		if (MeleeAnim && C->GetMesh() && C->GetMesh()->GetAnimInstance())
			C->GetMesh()->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(MeleeAnim, FName("DefaultSlot"));
	}
}

// RPC stubs
void AArenaPlayerController::Client_WaveStart_Implementation(int32 N) { BP_OnWaveStart(N); }
void AArenaPlayerController::Client_GameWon_Implementation() { BP_OnGameWon(); }
void AArenaPlayerController::Client_GameOver_Implementation() { BP_OnGameOver(); }
