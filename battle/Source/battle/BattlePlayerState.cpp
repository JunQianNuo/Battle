// BattlePlayerState.cpp

#include "BattlePlayerState.h"
#include "Net/UnrealNetwork.h"

ABattlePlayerState::ABattlePlayerState()
{
	Kills = 0;
	Deaths = 0;
	PlayerScore = 0;
}

void ABattlePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABattlePlayerState, Kills);
	DOREPLIFETIME(ABattlePlayerState, Deaths);
	DOREPLIFETIME(ABattlePlayerState, PlayerScore);
}

void ABattlePlayerState::AddKill()
{
	if (!HasAuthority()) return;
	Kills++;
}

void ABattlePlayerState::AddDeath()
{
	if (!HasAuthority()) return;
	Deaths++;
}

void ABattlePlayerState::AddScore(int32 Points)
{
	if (!HasAuthority()) return;
	PlayerScore += Points;
}
