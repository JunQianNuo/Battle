// EnemyBase.h - Extended enemy with type, melee, scoring, and network replication

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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FVector GetWeaponTargetLocation() override;

	// === Enemy configuration (replicated) ===

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Shooter;

	/** Score awarded to the killer */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 ScoreValue = 100;

	/** Movement speed multiplier (1.0 = default) */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float SpeedMultiplier = 1.0f;

	// === Melee config ===

	/** If true, this enemy uses melee attacks instead of ranged */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Melee")
	bool bIsMelee = false;

	/** Damage dealt on melee hit */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeDamage = 25.0f;

	/** Range for melee attacks */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeRange = 150.0f;

	/** Cooldown between melee attacks */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeCooldown = 1.0f;

	/** Knockback force on melee hit */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float MeleeKnockback = 500.0f;

	/** Melee attack animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	UAnimMontage* MeleeMontage;

	/** Max distance for ranged enemies to start shooting */
	UPROPERTY(Replicated, EditAnywhere, Category = "Enemy")
	float ShootRange = 1500.0f;

	// NOTE: CurrentHP inherited from AShooterNPC (public, BlueprintReadOnly)
	// NOTE: bIsDead inherited from AShooterNPC (protected, set by Die())

protected:
	FTimerHandle MeleeTimer;
	FTimerHandle DmgTimer;
	bool bMeleeReady = true;
	float LastMeleeTime = 0.0f;
	float LastPathTime = -999.0f;

	/** Controller that last damaged this enemy (used for kill credit) */
	UPROPERTY()
	TObjectPtr<AController> LastDamager;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void Die() override;
	virtual bool ShouldIncrementTeamScore() const override { return false; }

	void DoMeleeAttack(AActor* Target);
	void ResetMeleeCooldown();

	UFUNCTION()
	void OnEnemyOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Multicast: notify all clients this enemy died (play effects, ragdoll) */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_OnDeath();

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void MeleeAttack(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsMeleeReady() const { return bMeleeReady; }

	/** Returns whether this enemy is dead (reads the parent's bIsDead) */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bIsDead; }

	void SetWeaponClass(TSubclassOf<class AShooterWeapon> InClass) { WeaponClass = InClass; }
};
