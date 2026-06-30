// EnemyAnimInstance.h - C++ AnimInstance that drives enemy locomotion programmatically
// Exposes Speed/IsMoving for BP use, and plays run/idle animations from C++

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

class UAnimSequence;
class UAnimMontage;

UCLASS()
class BATTLE_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UEnemyAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// --- Movement data (BlueprintReadOnly for ABP use) ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float VerticalSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

	// --- Combat data ---

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float HPNormalized = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsDead = false;

	/** Set to true for unarmed (melee) enemies — determines which anim set to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bUseUnarmedAnims = true;

protected:
	UPROPERTY()
	TObjectPtr<ACharacter> CachedCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CachedMovement;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveThreshold = 10.0f;

	// --- Cached animation assets ---

	UPROPERTY()
	TObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequence> JogAnim;

	// --- Montage state ---

	UPROPERTY()
	TObjectPtr<UAnimMontage> CurrentMontage;

	bool bWasMoving = false;
	float BlendWeight = 0.0f;

	/** Load animation assets for the given type */
	void LoadAnimations(bool bUnarmed);

	/** Play a looping animation on the default slot */
	void PlayLocomotionAnim(UAnimSequence* Anim);
};
