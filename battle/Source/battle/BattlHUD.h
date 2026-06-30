// BattlHUD.h - Simple screen HUD
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BattlHUD.generated.h"

UCLASS()
class BATTLE_API ABattlHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
};
