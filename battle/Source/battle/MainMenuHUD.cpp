// MainMenuHUD.cpp
#include "MainMenuHUD.h"
#include "ArenaGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->SetInputMode(FInputModeGameAndUI());
		PC->bShowMouseCursor = true;
	}

	// Layout buttons centered on screen
	const float CenterX = 960.0f; // assume 1920x1080, will be overridden in DrawHUD
	const float BW = 300.0f, BH = 60.0f;
	const float StartY = 400.0f, Gap = 80.0f;

	Buttons.Add({TEXT("HOST GAME"), 0, StartY, BW, BH, &DoHost});
	Buttons.Add({TEXT("JOIN GAME"), 0, StartY + Gap, BW, BH, &DoJoin});
	Buttons.Add({TEXT("QUIT"), 0, StartY + Gap * 2, BW, BH, &DoQuit});
}

void AMainMenuHUD::DrawHUD()
{
	Super::DrawHUD();

	// Title
	const FString Title = TEXT("BATTLE ARENA");
	float TW, TH;
	GetTextSize(Title, TW, TH, nullptr, 4.0f);
	DrawText(Title, FColor::Red, (Canvas->SizeX - TW) / 2.0f, 150.0f, nullptr, 4.0f);

	// Subtitle
	const FString Sub = TEXT("Survive 2 minutes. Enemies every 20 seconds.");
	GetTextSize(Sub, TW, TH, nullptr, 1.5f);
	DrawText(Sub, FColor::White, (Canvas->SizeX - TW) / 2.0f, 240.0f, nullptr, 1.5f);

	// IP input prompt
	if (bWaitingForIP)
	{
		const FString Prompt = TEXT("Enter server IP and press Enter, or Escape to cancel");
		GetTextSize(Prompt, TW, TH, nullptr, 1.5f);
		DrawText(Prompt, FColor::Yellow, (Canvas->SizeX - TW) / 2.0f, 550.0f, nullptr, 1.5f);
		DrawText(JoinIP + TEXT("_"), FColor::White, (Canvas->SizeX - 200) / 2.0f, 580.0f, nullptr, 2.0f);
		return;
	}

	// Menu buttons
	const float CenterX = Canvas->SizeX / 2.0f;
	const float BW = 300.0f, BH = 60.0f;
	const float StartY = 400.0f, Gap = 80.0f;

	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		float X = CenterX - BW / 2.0f;
		float Y = StartY + i * Gap;
		Buttons[i].X = X;
		Buttons[i].Y = Y;
		FColor Color = (i == HoveredIndex) ? FColor::Yellow : FColor(100, 100, 255);
		DrawButton(Buttons[i].Text, X, Y, BW, BH, Color, FColor::Yellow);
	}

	// Mouse hover detection
	if (APlayerController* PC = GetOwningPlayerController())
	{
		float MX, MY;
		PC->GetMousePosition(MX, MY);
		HoveredIndex = -1;
		for (int32 i = 0; i < Buttons.Num(); ++i)
		{
			if (MX >= Buttons[i].X && MX <= Buttons[i].X + Buttons[i].W &&
				MY >= Buttons[i].Y && MY <= Buttons[i].Y + Buttons[i].H)
			{
				HoveredIndex = i;
				if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
				{
					Buttons[i].Action(this);
				}
				break;
			}
		}
	}

	// Keyboard input for IP
	if (bWaitingForIP)
	{
		const FKey NumKeys[10] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		for (int32 i = 0; i < 10; i++)
		{
			if (GetOwningPlayerController()->WasInputKeyJustPressed(NumKeys[i]))
				JoinIP += FString::Printf(TEXT("%d"), i);
		}
		if (GetOwningPlayerController()->WasInputKeyJustPressed(EKeys::Period))
			JoinIP += TEXT(".");
		if (GetOwningPlayerController()->WasInputKeyJustPressed(EKeys::BackSpace) && JoinIP.Len() > 0)
			JoinIP = JoinIP.LeftChop(1);
		if (GetOwningPlayerController()->WasInputKeyJustPressed(EKeys::Enter) && !JoinIP.IsEmpty())
		{
			if (APlayerController* PC2 = GetOwningPlayerController())
				PC2->ClientTravel(JoinIP, ETravelType::TRAVEL_Absolute);
		}
		if (GetOwningPlayerController()->WasInputKeyJustPressed(EKeys::Escape))
			bWaitingForIP = false;
	}
}

void AMainMenuHUD::DrawButton(const FString& Text, float X, float Y, float W, float H, FColor NormalColor, FColor HoverColor)
{
	// Draw border
	DrawRect(NormalColor, X, Y, W, H);

	// Draw inner
	DrawRect(FColor(20, 20, 40), X + 2, Y + 2, W - 4, H - 4);

	// Draw text
	float TW, TH;
	GetTextSize(Text, TW, TH, nullptr, 2.0f);
	DrawText(Text, NormalColor, X + (W - TW) / 2.0f, Y + (H - TH) / 2.0f, nullptr, 2.0f);
}

void AMainMenuHUD::HostGame()
{
	// Travel to shooter level with listen server + ArenaGameMode
	GetWorld()->ServerTravel(TEXT("Lvl_Shooter?listen?Game=/Script/battle.ArenaGameMode"));
}

void AMainMenuHUD::JoinGame()
{
	JoinIP = TEXT("127.0.0.1");
	bWaitingForIP = true;
}

void AMainMenuHUD::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayerController(), EQuitPreference::Quit, false);
}
