#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DECLARE_LOG_CATEGORY_EXTERN(LogScenario, Log, All);

struct AutoMove 
{
    float timestamp;
    float duration;
    FVector Vector;
};

struct AutoShoot 
{
    float timestamp;
    FString button;
};

class THIRDPERSON_API UScenario
{
public:
    UScenario();
    ~UScenario();

    void InitializeScenarioFromArgs();
    FVector GetMove(double Time, FVector CurrentLocation);
    bool HandleShooting(double time, APlayerController* PlayerController); // Pass PlayerController

private:
    TArray<AutoMove> Movements;
    TArray<AutoShoot> Shootings;

    void DeserializeJsonFile(const FString& FilePath);
    void SimulateMouseClick(FKey Key, APlayerController* PlayerController); // Pass PlayerController
};
