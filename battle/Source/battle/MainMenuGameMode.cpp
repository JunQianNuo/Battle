// MainMenuGameMode.cpp
#include "MainMenuGameMode.h"
#include "MainMenuHUD.h"
#include "battleCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	HUDClass = AMainMenuHUD::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterCharacter.BP_ShooterCharacter_C"));
	if (PawnClass.Succeeded()) { DefaultPawnClass = PawnClass.Class; }

	static ConstructorHelpers::FClassFinder<APlayerController> PCBP(TEXT("/Game/Variant_Shooter/Blueprints/BP_ShooterPlayerController.BP_ShooterPlayerController_C"));
	if (PCBP.Succeeded()) { PlayerControllerClass = PCBP.Class; }

	// Spawn weapon pickups at PlayerStarts (grab before entering arena)
	// Done in BeginPlay via default behavior
}
