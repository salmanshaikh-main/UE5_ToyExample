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

    // Function to find network details of the connected player
    virtual void PostLogin(APlayerController* NewPlayer) override;
    
    // Function to simulate a Denial of Service (DoS) attack on a targeted player
    UFUNCTION(BlueprintCallable, Category = "GameMode")
    void SimulateDoSAttack(APlayerController* TargetedPlayerController, int OutLossRate, int InLossRate, float Duration);
};