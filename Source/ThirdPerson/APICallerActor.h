#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "APICallerActor.generated.h"

class ACharacter;

UCLASS()
class THIRDPERSON_API AAPICallerActor : public AActor
{
    GENERATED_BODY()

public:
    AAPICallerActor();

    virtual void BeginPlay() override;

    void CallAPI(const FString& PlayerID);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnResponseReceived(const FString& PlayerID, const FString& ResponseString);


private:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* RootSceneComponent;

    FString CallingPlayerID;

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    UPROPERTY(EditAnywhere, Category = "API")
    float DistanceThreshold = 1000.f;
};