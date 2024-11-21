// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThirdPersonCharacter.h"
#include "CustomDepthActor.h"
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

    UFUNCTION()
    void SpawnObjectForRandomPlayer();

    // Property to set in Blueprint editor for spawn location
    UPROPERTY(EditAnywhere, Category = "Spawning")
    FVector SpawnLocation = FVector(2889.0f, 1446.0f, 134.15f);
    UPROPERTY(EditAnywhere, Category = "Spawning")
    FVector SpawnLocation2 = FVector(4346.0f, 1435.0f, 134.15f);

    // Property to set in Blueprint editor for spawn rotation
    UPROPERTY(EditAnywhere, Category = "Spawning")
    FRotator SpawnRotation = FRotator(0.0f, 0.0f, 0.0f);
    UPROPERTY(EditAnywhere, Category = "Spawning")
    FRotator SpawnRotation2 = FRotator(0.0f, 0.0f, 0.0f);

    // Property to set the actor class to spawn (make sure to set this to your CustomDepthActor in Blueprint)
    UPROPERTY(EditAnywhere, Category = "Spawning")
    TSubclassOf<AActor> ActorClassToSpawn = ACustomDepthActor::StaticClass();

};