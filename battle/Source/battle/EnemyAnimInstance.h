// EnemyAnimInstance.h - C++ AnimInstance that exposes movement data for enemy ABPs
// Fixes enemy locomotion: ABPs can read Speed/IsMoving without manual variable setup

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

UCLASS()
class BATTLE_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UEnemyAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Normalized ground speed (XY magnitude) — drive locomotion BlendSpace */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed = 0.0f;

	/** True when moving faster than walk threshold */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	/** Movement direction angle relative to actor forward (degrees) — drive strafe/normal blend */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction = 0.0f;

	/** Vertical velocity (positive = jumping/falling up) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float VerticalSpeed = 0.0f;

	/** True when falling */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

	/** Current HP percentage (0-1), driven from AEnemyBase */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float HPNormalized = 1.0f;

	/** True when dead (ragdoll/cleanup phase) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDead = false;

protected:
	UPROPERTY()
	TObjectPtr<class ACharacter> CachedCharacter;

	UPROPERTY()
	TObjectPtr<class UCharacterMovementComponent> CachedMovement;

	/** Speed threshold to consider as "moving" */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveThreshold = 10.0f;
};
