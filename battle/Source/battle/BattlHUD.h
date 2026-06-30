// BattlHUD.h - In-game HUD + Main Menu overlay
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

private:
	// Menu state
	bool bWaitingForIP = false;
	FString JoinIP;
	float LobbyStartTime = 0.0f;

	// Drawing helpers
	void DrawMenuScreen();
	void DrawLobbyScreen();
	void DrawInGameHUD();
	void DrawVictoryScreen(int32 Score);
	void DrawButton(const FString& Text, float X, float Y, float W, float H,
		FColor BaseColor, FColor HoverColor, bool bHover, bool bShadow = true);
	void DrawTextBox(float X, float Y, float W, float H, const FString& Text);

	bool IsHovering(float MX, float MY, float X, float Y, float W, float H) const;
};
