// BattlHUD.cpp — In-game HUD + Main Menu overlay
#include "BattlHUD.h"
#include "ArenaGameMode.h"
#include "BattleGameState.h"
#include "BattlePlayerState.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// ─── Drawing helpers ──────────────────────────────────────────────

bool ABattlHUD::IsHovering(float MX, float MY, float X, float Y, float W, float H) const
{
	return MX >= X && MX <= X + W && MY >= Y && MY <= Y + H;
}

void ABattlHUD::DrawButton(const FString& Text, float X, float Y, float W, float H,
	FColor BaseColor, FColor HoverColor, bool bHover, bool bShadow)
{
	FColor BorderColor = bHover ? HoverColor : BaseColor;
	FColor FillColor   = bHover ? FColor(30, 30, 60) : FColor(15, 15, 35);

	// Outer glow on hover
	if (bHover)
		DrawRect(FColor(BaseColor.R/4, BaseColor.G/4, BaseColor.B/4, 180), X - 3, Y - 3, W + 6, H + 6);

	// Border
	DrawRect(BorderColor, X, Y, W, H);

	// Fill
	DrawRect(FillColor, X + 2, Y + 2, W - 4, H - 4);

	// Text centered
	float TW, TH;
	GetTextSize(Text, TW, TH, nullptr, 2.0f);
	DrawText(Text, bHover ? FColor::White : FColor(200, 200, 230),
		X + (W - TW) / 2.0f, Y + (H - TH) / 2.0f, nullptr, 2.0f);
}

void ABattlHUD::DrawTextBox(float X, float Y, float W, float H, const FString& Text)
{
	DrawRect(FColor::White, X, Y, W, H);
	DrawRect(FColor(5, 5, 25), X + 2, Y + 2, W - 4, H - 4);

	float TW, TH;
	GetTextSize(Text, TW, TH, nullptr, 2.0f);
	DrawText(Text, FColor(100, 255, 100),
		X + 8, Y + (H - TH) / 2.0f, nullptr, 2.0f);

	// Blinking cursor
	if (FMath::Fmod(GetWorld()->GetTimeSeconds(), 1.0f) < 0.5f)
	{
		float CX = X + 8 + TW + 2;
		DrawText(TEXT("|"), FColor::Green, CX, Y + (H - TH) / 2.0f, nullptr, 2.0f);
	}
}

// ─── Main DrawHUD ─────────────────────────────────────────────────

void ABattlHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	// ── VICTORY SCREEN ─────────────────────────────────────────
	ABattleGameState* GS = PC->GetWorld()->GetGameState<ABattleGameState>();
	if (GS && GS->bGameWon)
	{
		DrawVictoryScreen(GS->TotalScore);
		return;
	}

	// ── MAIN MENU / LOBBY ──────────────────────────────────────
	// bInMenu is replicated via BattleGameState — works for both server and clients
	if (GS && GS->bInMenu)
	{
		AArenaGameMode* GM = Cast<AArenaGameMode>(GetWorld()->GetAuthGameMode());
		const int32 PlayerCount = GM ? GM->GetConnectedPlayers() : 1;
		if (PlayerCount > 1)
			DrawLobbyScreen();
		else
			DrawMenuScreen();
		return;
	}

	// ── IN-GAME HUD ────────────────────────────────────────────
	DrawInGameHUD();
}

// ─── Menu Screen (single player) ──────────────────────────────────

void ABattlHUD::DrawMenuScreen()
{
	APlayerController* PC = GetOwningPlayerController();
	AArenaGameMode* GM = Cast<AArenaGameMode>(GetWorld()->GetAuthGameMode());
	if (!PC || !GM) return;

	PC->SetInputMode(FInputModeGameAndUI());
	PC->bShowMouseCursor = true;

	const float CX = Canvas->SizeX / 2.0f;
	const float BW = 320.0f, BH = 55.0f;

	// Dark background overlay
	DrawRect(FColor(0, 0, 0, 180), 0, 0, Canvas->SizeX, Canvas->SizeY);

	// Title with pulse
	float Pulse = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * 0.05f;
	float TW, TH;
	const FString Title = TEXT("BATTLE  ARENA");
	GetTextSize(Title, TW, TH, nullptr, 5.0f);
	DrawText(Title, FColor(255, 60, 30), (Canvas->SizeX - TW * Pulse) / 2.0f, 100.0f,
		nullptr, 5.0f * Pulse);

	// Subtitle
	const FString Sub = TEXT("Survive 2 minutes.  Enemies spawn every 20 seconds.");
	GetTextSize(Sub, TW, TH, nullptr, 1.5f);
	DrawText(Sub, FColor(200, 200, 200), (Canvas->SizeX - TW) / 2.0f, 190.0f, nullptr, 1.5f);

	// IP Input mode
	if (bWaitingForIP)
	{
		const FString Prompt = TEXT("Enter server IP and press ENTER  |  ESC to cancel");
		GetTextSize(Prompt, TW, TH, nullptr, 1.4f);
		DrawText(Prompt, FColor::Yellow, (Canvas->SizeX - TW) / 2.0f, 330.0f, nullptr, 1.4f);

		DrawTextBox(CX - 150, 370, 300, 40, JoinIP + TEXT("_"));

		// Keyboard input
		const FKey NumKeys[10] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three,
			EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		for (int32 i = 0; i < 10; i++)
			if (PC->WasInputKeyJustPressed(NumKeys[i]))
				JoinIP += FString::Printf(TEXT("%d"), i);
		if (PC->WasInputKeyJustPressed(EKeys::Period))
			JoinIP += TEXT(".");
		if (PC->WasInputKeyJustPressed(EKeys::BackSpace) && JoinIP.Len() > 0)
			JoinIP = JoinIP.LeftChop(1);
		if (PC->WasInputKeyJustPressed(EKeys::Enter) && !JoinIP.IsEmpty())
		{
			PC->ClientTravel(JoinIP, ETravelType::TRAVEL_Absolute);
			return;
		}
		if (PC->WasInputKeyJustPressed(EKeys::Escape))
		{
			bWaitingForIP = false;
			JoinIP.Empty();
		}
		return;
	}

	// Menu buttons
	const float ButtonsY[4] = { 300.0f, 375.0f, 450.0f, 540.0f };
	const FString Labels[4] = {
		TEXT("START GAME"),
		TEXT("HOST GAME"),
		TEXT("JOIN GAME"),
		TEXT("QUIT")
	};
	const FColor Colors[4] = {
		FColor(60, 220, 60),
		FColor(80, 160, 255),
		FColor(255, 180, 60),
		FColor(220, 60, 60)
	};

	float MX, MY;
	PC->GetMousePosition(MX, MY);

	for (int32 i = 0; i < 4; ++i)
	{
		float X = CX - BW / 2.0f;
		float Y = ButtonsY[i];
		bool bHover = IsHovering(MX, MY, X, Y, BW, BH);

		DrawButton(Labels[i], X, Y, BW, BH, Colors[i], Colors[i], bHover);

		if (bHover && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			switch (i)
			{
			case 0: GM->StartGame(); break;
			case 1: GM->HostGame(TEXT("Lvl_Shooter")); break;
			case 2: JoinIP = TEXT("127.0.0.1"); bWaitingForIP = true; break;
			case 3: UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false); break;
			}
		}
	}

	// Version info
	const FString Ver = TEXT("v0.2  |  UE 5.8");
	GetTextSize(Ver, TW, TH, nullptr, 1.0f);
	DrawText(Ver, FColor(100, 100, 100), Canvas->SizeX - TW - 10, Canvas->SizeY - 25, nullptr, 1.0f);
}

// ─── Lobby Screen (multiplayer host) ───────────────────────────────

void ABattlHUD::DrawLobbyScreen()
{
	APlayerController* PC = GetOwningPlayerController();
	AArenaGameMode* GM = Cast<AArenaGameMode>(GetWorld()->GetAuthGameMode());
	if (!PC || !GM) return;

	PC->SetInputMode(FInputModeGameAndUI());
	PC->bShowMouseCursor = true;

	const float CX = Canvas->SizeX / 2.0f;
	const float BW = 320.0f, BH = 55.0f;

	// Dark overlay
	DrawRect(FColor(0, 0, 0, 200), 0, 0, Canvas->SizeX, Canvas->SizeY);

	// Title
	float TW, TH;
	const FString Title = TEXT("GAME  LOBBY");
	GetTextSize(Title, TW, TH, nullptr, 4.0f);
	DrawText(Title, FColor(80, 160, 255), (Canvas->SizeX - TW) / 2.0f, 100.0f, nullptr, 4.0f);

	// Player list
	const int32 PlayerCount = GM->GetConnectedPlayers();
	const FString PlayerLine = FString::Printf(TEXT("Players Connected:  %d  /  %d"), PlayerCount, GM->MaxPlayers);
	GetTextSize(PlayerLine, TW, TH, nullptr, 2.0f);
	DrawText(PlayerLine, FColor::White, (Canvas->SizeX - TW) / 2.0f, 220.0f, nullptr, 2.0f);

	// Waiting indicator
	float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.5f + 0.5f;
	FString WaitText = TEXT("Waiting for players...");
	GetTextSize(WaitText, TW, TH, nullptr, 1.5f);
	DrawText(WaitText,
		FColor(FMath::Lerp(80, 200, Pulse), FMath::Lerp(80, 200, Pulse), FMath::Lerp(80, 200, Pulse)),
		(CX - TW) / 2.0f, 280.0f, nullptr, 1.5f);

	// Buttons
	float MX, MY;
	PC->GetMousePosition(MX, MY);

	const float ButtonsY[2] = { 370.0f, 445.0f };
	const FString Labels[2] = { TEXT("START GAME"), TEXT("CANCEL / QUIT") };
	const FColor Colors[2] = { FColor(60, 220, 60), FColor(220, 60, 60) };

	for (int32 i = 0; i < 2; ++i)
	{
		float X = CX - BW / 2.0f;
		float Y = ButtonsY[i];
		bool bHover = IsHovering(MX, MY, X, Y, BW, BH);

		DrawButton(Labels[i], X, Y, BW, BH, Colors[i], Colors[i], bHover);

		if (bHover && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			if (i == 0)
				GM->StartGame();
			else
				GM->HostGame(TEXT("Lvl_Shooter")); // Actually just restart
		}
	}

	// Tip
	const FString Tip = TEXT("Other players: open console (~) and type  join <host-ip>");
	GetTextSize(Tip, TW, TH, nullptr, 1.0f);
	DrawText(Tip, FColor(150, 150, 150), (Canvas->SizeX - TW) / 2.0f, 540.0f, nullptr, 1.0f);
}

// ─── Victory Screen ────────────────────────────────────────────────

void ABattlHUD::DrawVictoryScreen(int32 Score)
{
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}

	// Dark overlay
	DrawRect(FColor(0, 0, 0, 160), 0, 0, Canvas->SizeX, Canvas->SizeY);

	const FColor Gold(255, 215, 0);
	const FString Line1 = TEXT("*** VICTORY! ***");
	const FString Line2 = FString::Printf(TEXT("Final Score:  %d"), Score);
	const FString Line3 = TEXT("Press ESC or click to return");

	float S1W, S1H, S2W, S2H, S3W, S3H;
	GetTextSize(Line1, S1W, S1H, nullptr, 6.0f);
	GetTextSize(Line2, S2W, S2H, nullptr, 3.0f);
	GetTextSize(Line3, S3W, S3H, nullptr, 1.5f);

	float Y = Canvas->SizeY * 0.28f;

	// Pulsing victory text
	float Pulse = 1.0f + FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) * 0.08f;
	DrawText(Line1, Gold,
		(Canvas->SizeX - S1W * Pulse) / 2.0f, Y, nullptr, 6.0f * Pulse);

	Y += S1H + 20.0f;
	DrawText(Line2, FColor::White, (Canvas->SizeX - S2W) / 2.0f, Y, nullptr, 3.0f);

	Y += S2H + 40.0f;
	DrawText(Line3, FColor(150, 150, 150), (Canvas->SizeX - S3W) / 2.0f, Y, nullptr, 1.5f);
}

// ─── In-Game HUD ──────────────────────────────────────────────────

void ABattlHUD::DrawInGameHUD()
{
	ABattleGameState* GS = GetWorld()->GetGameState<ABattleGameState>();
	if (!GS) return;

	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	// ── Timer — top-center ────────────────────────────────────
	int32 Min = FMath::FloorToInt(GS->RemainingTime / 60.0f);
	int32 Sec = FMath::FloorToInt(FMath::Fmod(GS->RemainingTime, 60.0f));
	FString TimerText = FString::Printf(TEXT("%d:%02d"), Min, Sec);
	FColor TimerColor = GS->RemainingTime <= 30.0f ? FColor::Red : FColor::White;

	float TW2, TH2;
	GetTextSize(TimerText, TW2, TH2, nullptr, 4.0f);
	DrawText(TimerText, TimerColor, (Canvas->SizeX - TW2) / 2.0f, 30.0f, nullptr, 4.0f);

	// ── Score / Kills / Deaths — top-left ──────────────────────
	float X = Canvas->SizeX * 0.02f;
	float Y = 140.0f;

	ABattlePlayerState* PS = PC ? PC->GetPlayerState<ABattlePlayerState>() : nullptr;
	FString Text = FString::Printf(TEXT("SCORE: %d  |  KILLS: %d  |  DEATHS: %d"),
		GS->TotalScore,
		PS ? PS->Kills : 0,
		PS ? PS->Deaths : 0);
	DrawText(Text, FColor::Green, X, Y, nullptr, 1.5f);

	// ── Enemies Remaining — top-right ──────────────────────────
	FString EnemyText = FString::Printf(TEXT("ENEMIES: %d"), GS->EnemiesRemaining);
	float EW, EH;
	GetTextSize(EnemyText, EW, EH, nullptr, 1.5f);
	DrawText(EnemyText, FColor(255, 180, 80), Canvas->SizeX - EW - 20, Y, nullptr, 1.5f);

	// ── Health bar — bottom-center ─────────────────────────────
	if (PS)
	{
		float BarW = 300.0f, BarH = 16.0f;
		float BarX = (Canvas->SizeX - BarW) / 2.0f;
		float BarY = Canvas->SizeY - 40.0f;

		DrawRect(FColor(60, 60, 60), BarX, BarY, BarW, BarH);

		float HP = 1.0f - PS->Deaths * 0.2f; // rough HP estimate
		HP = FMath::Clamp(HP, 0.0f, 1.0f);
		FColor HPColor = HP > 0.5f ? FColor(50, 200, 50) : (HP > 0.25f ? FColor(200, 200, 50) : FColor(200, 50, 50));
		DrawRect(HPColor, BarX, BarY, BarW * HP, BarH);
	}
}
