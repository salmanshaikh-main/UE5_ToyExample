// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonGameMode.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerState.h"

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
