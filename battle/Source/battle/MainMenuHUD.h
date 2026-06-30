// MainMenuHUD.h - Simple Canvas-based main menu
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

UCLASS()
class BATTLE_API AMainMenuHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void HostGame();

	UFUNCTION(BlueprintCallable)
	void JoinGame();

	UFUNCTION(BlueprintCallable)
	void QuitGame();

private:
	void DrawButton(const FString& Text, float X, float Y, float W, float H, FColor NormalColor, FColor HoverColor);

	FString JoinIP;
	bool bWaitingForIP = false;

	struct FMenuButton
	{
		FString Text;
		float X, Y, W, H;
		void (*Action)(AMainMenuHUD*);
	};
	TArray<FMenuButton> Buttons;
	int32 HoveredIndex = -1;

	static void DoHost(AMainMenuHUD* HUD) { HUD->HostGame(); }
	static void DoJoin(AMainMenuHUD* HUD) { HUD->JoinGame(); }
	static void DoQuit(AMainMenuHUD* HUD) { HUD->QuitGame(); }
};
