// BattlePlayerState.h - Per-player replicated state

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BattlePlayerState.generated.h"

UCLASS()
class BATTLE_API ABattlePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABattlePlayerState();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Kills;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 Deaths;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Score")
	int32 PlayerScore;

	void AddKill();
	void AddDeath();
	void AddScore(int32 Points);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
