// EnemyAnimInstance.cpp — C++ driven enemy locomotion (idle ↔ jog)

#include "EnemyAnimInstance.h"
#include "EnemyBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Kismet/KismetMathLibrary.h"

UEnemyAnimInstance::UEnemyAnimInstance()
{
}

void UEnemyAnimInstance::LoadAnimations(bool bUnarmed)
{
	if (bUnarmed)
	{
		// Unarmed (melee enemies: Runner, Tank)
		IdleAnim = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")));
		JogAnim  = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd")));
	}
	else
	{
		// Pistol (ranged enemy: Shooter) — use pistol idle + jog
		IdleAnim = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Pistol/MF_Pistol_Idle_ADS.MF_Pistol_Idle_ADS")));
		JogAnim  = Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr,
			TEXT("/Game/Characters/Mannequins/Anims/Pistol/Jog/MF_Pistol_Jog_Fwd.MF_Pistol_Jog_Fwd")));
	}

	if (!IdleAnim)
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAnim] Failed to load Idle animation"));
	if (!JogAnim)
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAnim] Failed to load Jog animation"));
}

void UEnemyAnimInstance::PlayLocomotionAnim(UAnimSequence* Anim)
{
	if (!Anim) return;

	// Stop current montage if playing
	if (CurrentMontage)
	{
		Montage_Stop(0.15f, CurrentMontage);
		CurrentMontage = nullptr;
	}

	// Play as looping montage on DefaultSlot (full body)
	// BlendIn=0.15, BlendOut=0.15, LoopCount=-1 = infinite loop
	float const BlendTime = 0.15f;
	CurrentMontage = PlaySlotAnimationAsDynamicMontage(
		Anim, FName("DefaultSlot"),
		BlendTime,   // BlendIn
		BlendTime,   // BlendOut
		1.0f,        // PlayRate
		-1,          // LoopCount (-1 = infinite)
		0.0f,        // BlendOutTriggerTime
		0.0f         // StartTime
	);
}

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* Owner = GetOwningActor())
	{
		CachedCharacter = Cast<ACharacter>(Owner);
		if (CachedCharacter != nullptr)
		{
			CachedMovement = CachedCharacter->GetCharacterMovement();

			// Determine animation set from enemy type
			if (AEnemyBase* Enemy = Cast<AEnemyBase>(CachedCharacter.Get()))
			{
				bUseUnarmedAnims = Enemy->bIsMelee;
			}
		}
	}

	LoadAnimations(bUseUnarmedAnims);

	// Start idle
	if (IdleAnim)
	{
		PlayLocomotionAnim(IdleAnim);
	}

	bWasMoving = false;
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Refresh cached pointers
	if (!CachedCharacter)
	{
		if (APawn* Pawn = TryGetPawnOwner())
		{
			CachedCharacter = Cast<ACharacter>(Pawn);
			if (CachedCharacter != nullptr)
			{
				CachedMovement = CachedCharacter->GetCharacterMovement();
			}
		}
	}

	if (!CachedCharacter)
	{
		return;
	}

	// --- Movement data ---
	const FVector Velocity = CachedCharacter->GetVelocity();
	Speed = Velocity.Size2D();
	bIsMoving = Speed > MoveThreshold;

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

	VerticalSpeed = Velocity.Z;
	bIsFalling = (CachedMovement != nullptr) && CachedMovement->IsFalling();

	// --- Combat data ---
	if (AEnemyBase* Enemy = Cast<AEnemyBase>(CachedCharacter.Get()))
	{
		HPNormalized = FMath::Clamp(Enemy->CurrentHP / 100.0f, 0.0f, 1.0f);
	}
	bIsDead = CachedCharacter->ActorHasTag(FName("Dead"));

	// --- Locomotion animation switching ---
	// Only drive animation if not dead (ragdoll handles dead state)
	if (bIsDead)
	{
		return;
	}

	if (bIsMoving != bWasMoving)
	{
		// State changed — switch animation
		if (bIsMoving && JogAnim)
		{
			PlayLocomotionAnim(JogAnim);
		}
		else if (!bIsMoving && IdleAnim)
		{
			PlayLocomotionAnim(IdleAnim);
		}
		bWasMoving = bIsMoving;
	}
}
