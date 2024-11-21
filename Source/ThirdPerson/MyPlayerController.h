// MyPlayerController.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS()
class THIRDPERSON_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UFUNCTION(Client, Reliable)
    void Client_SpawnCustomDepthActor(const FVector Location, const FRotator Rotation, TSubclassOf<AActor> ActorClass);
};