// ArenaPlayerController.h - HUD events + player melee
#pragma once
#include "CoreMinimal.h"
#include "Variant_Shooter/ShooterPlayerController.h"
#include "ArenaPlayerController.generated.h"

UCLASS()
class BATTLE_API AArenaPlayerController : public AShooterPlayerController
{
	GENERATED_BODY()
public:
	AArenaPlayerController();

	UFUNCTION(Client, Reliable) void Client_WaveStart(int32 WaveNumber);
	UFUNCTION(Client, Reliable) void Client_GameWon();
	UFUNCTION(Client, Reliable) void Client_GameOver();
	UFUNCTION(BlueprintImplementableEvent) void BP_OnKillMessage(const FString& K, const FString& V);
	UFUNCTION(BlueprintImplementableEvent) void BP_OnWaveStart(int32 Wave);
	UFUNCTION(BlueprintImplementableEvent) void BP_OnGameWon();
	UFUNCTION(BlueprintImplementableEvent) void BP_OnGameOver();

protected:
	virtual void SetupInputComponent() override;
	void DoMelee();

	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeDamage = 25.0f;
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeRange = 200.0f;
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeCooldown = 0.5f;

	float LastMeleeTime = -999.0f;

	// Melee animation sequence (converted to montage at runtime)
	static UAnimSequence* MeleeAnim;
};
