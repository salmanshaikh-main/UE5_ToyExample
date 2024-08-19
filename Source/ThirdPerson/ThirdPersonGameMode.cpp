// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameSession.h"
#include "Engine/Engine.h"
#include "CustomPlayerController.h"

DEFINE_LOG_CATEGORY(LogThirdPersonGameMode);

AThirdPersonGameMode::AThirdPersonGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	/////// Surement ca qui fait planter
	//for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	//{
	//	APlayerController* PlayerController = It->Get();

	//	if (PlayerController)
	//	{
	//		FString NetworkAddr = PlayerController->GetPlayerNetworkAddress();
	//		UE_LOG(LogThirdPersonGameMode, Log, TEXT("Network address: %s"), &NetworkAddr);
	//	}
	//	else
	//	{
	//		UE_LOG(LogThirdPersonGameMode, Log, TEXT("PlayerController = NULL"));

	//	}
	//}

}

void AThirdPersonGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UNetConnection* IncomingNetConnection = Cast<UNetConnection>(NewPlayer->Player);
    if (!IsValid(IncomingNetConnection))
    {
        UE_LOG(LogThirdPersonGameMode, Error, TEXT("Invalid IncomingNetConnection for player: %s"), *NewPlayer->GetName());
        return;
    }

    TSharedPtr<const FInternetAddr> Addr = IncomingNetConnection->GetRemoteAddr();
    if (!Addr.IsValid())
    {
        UE_LOG(LogThirdPersonGameMode, Error, TEXT("Invalid remote address for player: %s"), *NewPlayer->GetName());
        return;
    }

    FString IPAddressString = Addr->ToString(false);
    UE_LOG(LogThirdPersonGameMode, Log, TEXT("Player IP Address: %s"), *IPAddressString);

    uint32 IPAddr;
    Addr->GetIp(IPAddr);
    UE_LOG(LogThirdPersonGameMode, Log, TEXT("Player IP (as uint32): %u"), IPAddr);

}

void AThirdPersonGameMode::SimulateDoSAttack(APlayerController* TargetedPlayerController, int OutLossRate, int InLossRate, float Duration)
{
    if (!TargetedPlayerController || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid PlayerController or World"));
        return;
    }
    if (AThirdPersonCharacter* TargetedCharacter = Cast<AThirdPersonCharacter>(TargetedPlayerController->GetPawn()))
    {
        TargetedCharacter->Client_SetPacketLoss(OutLossRate, InLossRate);
            // UE_LOG(LogTemp, Log, TEXT("Starting packet loss simulation on player: %s. Loss Rate: %f%%"), 
            //    *TargetedPlayerController->GetName(), LossRate);
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