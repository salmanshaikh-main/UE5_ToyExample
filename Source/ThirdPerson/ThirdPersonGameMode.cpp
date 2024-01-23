// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonGameMode.h"
#include "UObject/ConstructorHelpers.h"

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

	if (NewPlayer->IsLocalPlayerController())
	{
		return;
	}

	UNetConnection* IncomingNetConnection = Cast<UNetConnection>(NewPlayer->Player);

	TSharedPtr<const FInternetAddr> Addr = IncomingNetConnection->GetRemoteAddr();
	uint32 IPAddr;
	Addr->GetIp(IPAddr);
	UE_LOG(LogThirdPersonGameMode, Log, TEXT("IP: %d"), IPAddr);

	if (!IsValid(IncomingNetConnection) || !IncomingNetConnection->PlayerId.IsValid())
	{
		UE_LOG(LogThirdPersonGameMode, Log, TEXT("incomingNetConnection not valid"));
	}
	else
	{
		UE_LOG(LogThirdPersonGameMode, Log, TEXT("incomingNetConnection not valid"));
	}

}
