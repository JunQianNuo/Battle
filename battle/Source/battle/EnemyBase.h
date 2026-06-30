// EnemyBase.h - Extended enemy base with type, melee, and scoring support

#pragma once

#include "CoreMinimal.h"
#include "Variant_Shooter/AI/ShooterNPC.h"
#include "BattleGameState.h"
#include "EnemyBase.generated.h"

UCLASS()
class BATTLE_API AEnemyBase : public AShooterNPC
{
	GENERATED_BODY()

public:
	AEnemyBase();
	virtual void Tick(float DeltaTime) override;
	virtual FVector GetWeaponTargetLocation() override;

	// === Enemy configuration ===

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Shooter;

	/** Score awarded to the killer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 ScoreValue = 100;

	/** Movement speed multiplier (1.0 = default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float SpeedMultiplier = 1.0f;

	// === Melee config ===

	/** If true, this enemy uses melee attacks instead of ranged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	bool bIsMelee = false;

	/** Damage dealt on melee hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeDamage = 25.0f;

	/** Range for melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeRange = 150.0f;

	/** Cooldown between melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeCooldown = 1.0f;

	/** Knockback force on melee hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeKnockback = 500.0f;

	/** Melee attack animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	UAnimMontage* MeleeMontage;

	/** Max distance for ranged enemies to start shooting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float ShootRange = 1500.0f;

protected:
	FTimerHandle MeleeTimer;
	FTimerHandle DmgTimer; // melee damage delay timer (cleared on destroy)
	bool bMeleeReady = true;
	float LastMeleeTime = 0.0f;
	float LastPathTime = -999.0f;

	/** Controller that last damaged this enemy (used for kill credit) */
	UPROPERTY()
	TObjectPtr<AController> LastDamager;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Override Die to report kill to GameState */
	virtual void Die() override;
	virtual bool ShouldIncrementTeamScore() const override { return false; }

	/** Melee attack against a target */
	void DoMeleeAttack(AActor* Target);

	/** Reset melee cooldown */
	void ResetMeleeCooldown();

	/** Called when this enemy overlaps with another actor (for melee contact damage) */
	UFUNCTION()
	void OnEnemyOverlap(AActor* OverlappedActor, AActor* OtherActor);

public:
	/** Perform a melee attack (called from AI/StateTree) */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void MeleeAttack(AActor* Target);

	/** Returns true if melee attack is ready */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsMeleeReady() const { return bMeleeReady; }

	/** Set weapon class for ranged enemies */
	void SetWeaponClass(TSubclassOf<class AShooterWeapon> InClass) { WeaponClass = InClass; }
};
