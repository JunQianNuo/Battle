// EnemyAnimInstance.cpp

#include "EnemyAnimInstance.h"
#include "EnemyBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UEnemyAnimInstance::UEnemyAnimInstance()
{
}

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache pointers to avoid repeated casts
	if (AActor* Owner = GetOwningActor())
	{
		CachedCharacter = Cast<ACharacter>(Owner);
		if (CachedCharacter.IsValid())
		{
			CachedMovement = CachedCharacter->GetCharacterMovement();
		}
	}
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Refresh cached pointers if pawn changed
	if (!CachedCharacter.IsValid())
	{
		if (APawn* Pawn = TryGetPawnOwner())
		{
			CachedCharacter = Cast<ACharacter>(Pawn);
			if (CachedCharacter.IsValid())
			{
				CachedMovement = CachedCharacter->GetCharacterMovement();
			}
		}
	}

	if (!CachedCharacter.IsValid())
	{
		return;
	}

	// --- Movement data ---

	const FVector Velocity = CachedCharacter->GetVelocity();

	// Ground speed (XY magnitude)
	Speed = Velocity.Size2D();
	bIsMoving = Speed > MoveThreshold;

	// Movement direction relative to actor forward
	if (bIsMoving)
	{
		const FRotator ActorRotation = CachedCharacter->GetActorRotation();
		const FVector LocalVelocity = ActorRotation.UnrotateVector(Velocity);
		Direction = UKismetMathLibrary::MakeRotFromX(LocalVelocity).Yaw;
	}
	else
	{
		Direction = 0.0f;
	}

	// Vertical movement
	VerticalSpeed = Velocity.Z;
	bIsFalling = CachedMovement.IsValid() && CachedMovement->IsFalling();

	// --- Combat data from AEnemyBase ---

	if (AEnemyBase* Enemy = Cast<AEnemyBase>(CachedCharacter.Get()))
	{
		HPNormalized = FMath::Clamp(Enemy->CurrentHP / 100.0f, 0.0f, 1.0f); // Default max 100
	}

	// Death state: check Dead tag (set by ShooterNPC::Die)
	bIsDead = CachedCharacter->ActorHasTag(FName("Dead"));
}
