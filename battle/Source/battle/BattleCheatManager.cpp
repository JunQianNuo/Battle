// BattleCheatManager.cpp
#include "BattleCheatManager.h"
#include "ArenaGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UBattleCheatManager::Host(FString MapName)
{
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(GetWorld()->GetAuthGameMode()))
		GM->HostGame(MapName);
	else
	{
		// Fallback: direct server travel
		FString Command = MapName + TEXT("?listen");
		GetWorld()->ServerTravel(Command);
	}
}

void UBattleCheatManager::Join(FString IP)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		PC->ClientTravel(IP, ETravelType::TRAVEL_Absolute);
}
