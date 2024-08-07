#include "APICallerActor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AAPICallerActor::AAPICallerActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;
}

void AAPICallerActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAPICallerActor::CallAPI(const FString& PlayerID)
{
    CallingPlayerID = PlayerID;
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &AAPICallerActor::OnResponseReceived);
    Request->SetURL("http://localhost:5000/message");
    Request->SetVerb("GET");
    Request->ProcessRequest();
}

void AAPICallerActor::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (HasAuthority())
    {
        if (bWasSuccessful && Response.IsValid())
        {
            FString ResponseString = Response->GetContentAsString();
            UE_LOG(LogTemp, Log, TEXT("API Response for Player %s: %s"), *CallingPlayerID, *ResponseString);
            Multicast_OnResponseReceived(CallingPlayerID, ResponseString);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("API call failed for Player %s"), *CallingPlayerID);
        }
    }
}

void AAPICallerActor::Multicast_OnResponseReceived_Implementation(const FString& PlayerID, const FString& ResponseString)
{
    TArray<AActor*> FoundCharacters;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundCharacters);
    for (AActor* Actor : FoundCharacters)
    {
        ACharacter* Character = Cast<ACharacter>(Actor);
        if (Character)
        {
            float Distance = FVector::Dist(GetActorLocation(), Character->GetActorLocation());
			UE_LOG(LogTemp, Log, TEXT("Distance: %f"), Distance);
            if (Distance <= DistanceThreshold)
            {
                APlayerController* PC = Cast<APlayerController>(Character->GetController());
                if (PC)
                {
                    //UE_LOG(LogTemp, Log, TEXT("Displaying message for local controller: %s"), *PC->GetName());
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Player %s: %s"), *PlayerID, *ResponseString));
                }
            }
        }
    }
}

