// BattlHUD.cpp
#include "BattlHUD.h"
#include "ArenaGameMode.h"
#include "BattleGameState.h"
#include "BattlePlayerState.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

void ABattlHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	// === MAIN MENU ===
	if (AArenaGameMode* GM = Cast<AArenaGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (GM->bInMenu)
		{
			PC->SetInputMode(FInputModeGameAndUI());
			PC->bShowMouseCursor = true;

			const float CX = Canvas->SizeX / 2.0f;
			const float BW = 300.0f, BH = 55.0f;

			// Title
			float TW, TH;
			const FString Title = TEXT("BATTLE ARENA");
			GetTextSize(Title, TW, TH, nullptr, 4.0f);
			DrawText(Title, FColor::Red, (Canvas->SizeX - TW)/2.0f, 120.0f, nullptr, 4.0f);

			const FString Sub = TEXT("Survive 2 minutes. Enemies every 20s.");
			GetTextSize(Sub, TW, TH, nullptr, 1.5f);
			DrawText(Sub, FColor::White, (Canvas->SizeX - TW)/2.0f, 200.0f, nullptr, 1.5f);

			// Buttons
			const float ButtonsY[3] = { 350.0f, 420.0f, 490.0f };
			const FString Labels[3] = { TEXT("[ START GAME ]"), TEXT("[ HOST GAME ]"), TEXT("[ QUIT ]") };
			const FColor HoverColors[3] = { FColor::Green, FColor(100,150,255), FColor::Red };

			float MX, MY;
			PC->GetMousePosition(MX, MY);

			for (int32 i = 0; i < 3; ++i)
			{
				float X = CX - BW/2.0f;
				float Y = ButtonsY[i];
				bool bHover = (MX >= X && MX <= X+BW && MY >= Y && MY <= Y+BH);

				DrawRect(bHover ? HoverColors[i] : FColor(80,80,120), X, Y, BW, BH);
				GetTextSize(Labels[i], TW, TH, nullptr, 2.0f);
				DrawText(Labels[i], FColor::White, X + (BW-TW)/2.0f, Y + (BH-TH)/2.0f, nullptr, 2.0f);

				if (bHover && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
				{
					if (i == 0) GM->StartGame(); // START
					else if (i == 1) GM->HostGame(TEXT("Lvl_Shooter")); // HOST
					else if (i == 2) UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false); // QUIT
				}
			}

			return;
		}
	}

	// === VICTORY SCREEN ===
	ABattleGameState* GS = PC->GetWorld()->GetGameState<ABattleGameState>();
	if (!GS) return;
	if (GS->bGameWon)
	{
		const FColor Gold(255, 215, 0);
		const FString Line1 = TEXT("*** VICTORY! ***");
		const FString Line2 = FString::Printf(TEXT("Final Score: %d"), GS->TotalScore);

		float S1W, S1H, S2W, S2H;
		GetTextSize(Line1, S1W, S1H, nullptr, 5.0f);
		GetTextSize(Line2, S2W, S2H, nullptr, 3.0f);

		float X1 = (Canvas->SizeX - S1W) / 2.0f;
		float X2 = (Canvas->SizeX - S2W) / 2.0f;
		float Y1 = Canvas->SizeY * 0.30f;
		float Y2 = Y1 + S1H + 10.0f;

		DrawText(Line1, Gold, X1, Y1, nullptr, 5.0f);
		DrawText(Line2, FColor::White, X2, Y2, nullptr, 3.0f);
		return;
	}

	// === IN-GAME HUD ===
	// Timer — top center
	int32 Min = FMath::FloorToInt(GS->RemainingTime / 60.0f);
	int32 Sec = FMath::FloorToInt(FMath::Fmod(GS->RemainingTime, 60.0f));
	FString TimerText = FString::Printf(TEXT("%d:%02d"), Min, Sec);
	FColor TimerColor = GS->RemainingTime <= 30.0f ? FColor::Red : FColor::White;

	float TW2, TH2;
	GetTextSize(TimerText, TW2, TH2, nullptr, 4.0f);
	DrawText(TimerText, TimerColor, (Canvas->SizeX - TW2) / 2.0f, 30.0f, nullptr, 4.0f);

	// Score / Kills / Deaths — top-left
	float X = Canvas->SizeX * 0.02f;
	float Y = 140.0f;

	ABattlePlayerState* PS = PC->GetPlayerState<ABattlePlayerState>();
	FString Text = FString::Printf(TEXT("SCORE: %d  |  KILLS: %d  |  DEATHS: %d"),
		GS->TotalScore,
		PS ? PS->Kills : 0,
		PS ? PS->Deaths : 0);
	DrawText(Text, FColor::Green, X, Y, nullptr, 1.5f);
}
