// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameSession.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogThirdPersonGameMode);

AThirdPersonGameMode::AThirdPersonGameMode()
{
	// Set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

// After a player logs in, we can get the IP address of the player and log it to the console.
void AThirdPersonGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // Cast the NewPlayer's Player property to UNetConnection to access network connection details.
    UNetConnection* IncomingNetConnection = Cast<UNetConnection>(NewPlayer->Player);

    // Check if the cast was successful. If not, log an error and return early.
    if (!IsValid(IncomingNetConnection))
    {
        UE_LOG(LogThirdPersonGameMode, Error, TEXT("Invalid IncomingNetConnection for player: %s"), *NewPlayer->GetName());
        return;
    }

    // Retrieve the player's remote address as FInternetAddr.
    TSharedPtr<const FInternetAddr> Addr = IncomingNetConnection->GetRemoteAddr();

    // Check if the address is valid. If not, log an error and return early.
    if (!Addr.IsValid())
    {
        UE_LOG(LogThirdPersonGameMode, Error, TEXT("Invalid remote address for player: %s"), *NewPlayer->GetName());
        return;
    }

    // Convert the address to a human-readable string (e.g., "192.168.1.1") and log it.
    FString IPAddressString = Addr->ToString(false);
    UE_LOG(LogThirdPersonGameMode, Log, TEXT("Player IP Address: %s"), *IPAddressString);

    // Retrieve the IP address as a uint32 and log it for further use if needed.
    uint32 IPAddr;
    Addr->GetIp(IPAddr);
    UE_LOG(LogThirdPersonGameMode, Log, TEXT("Player IP (as uint32): %u"), IPAddr);
}

 // This function will simulate a Denial of Service (DoS) attack on a targeted player by introducing packet loss.
void AThirdPersonGameMode::SimulateDoSAttack(APlayerController* TargetedPlayerController, int OutLossRate, int InLossRate, float Duration)
{
    if (!TargetedPlayerController || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid PlayerController or World"));
        return;
    }
    // Cast the TargetedPlayerController's Pawn to AThirdPersonCharacter to access the Client_SetPacketLoss function.
    if (AThirdPersonCharacter* TargetedCharacter = Cast<AThirdPersonCharacter>(TargetedPlayerController->GetPawn()))
    {
        TargetedCharacter->Client_SetPacketLoss(OutLossRate, InLossRate);
    }
    // Set a timer to stop the simulation
    FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, TargetedPlayerController]()
	{
		if (TargetedPlayerController && TargetedPlayerController->IsValidLowLevel())
		{
			if (AThirdPersonCharacter* TargetedCharacter = Cast<AThirdPersonCharacter>(TargetedPlayerController->GetPawn()))
			{
				TargetedCharacter->Client_SetPacketLoss(0, 0);
				// UE_LOG(LogTemp, Log, TEXT("Packet loss simulation ended for player: %s"), *TargetedPlayerController->GetName());
			}
		}
	}, Duration, false);
}