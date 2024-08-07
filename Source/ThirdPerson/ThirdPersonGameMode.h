// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThirdPersonCharacter.h"
#include "ThirdPersonGameMode.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogThirdPersonGameMode, Log, All);

UCLASS(minimalapi)
class AThirdPersonGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AThirdPersonGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode")
    void OnPlayerPostLogin(APlayerController* NewPlayer);
	
};
