// MyPlayerController.cpp
#include "MyPlayerController.h"

void AMyPlayerController::Client_SpawnCustomDepthActor_Implementation(const FVector Location, const FRotator Rotation, TSubclassOf<AActor> ActorClass)
{
    if (!ActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid actor class"));
        return;
    }

    if (UWorld* World = GetWorld())
    {
        // Spawn the actor
        if (AActor* SpawnedActor = World->SpawnActor<AActor>(ActorClass, Location, Rotation))
        {
            UE_LOG(LogTemp, Log, TEXT("Spawned actor of class %s at location: %s"), 
                *ActorClass->GetName(), *Location.ToString());
            
            // Debug information for the mesh component
            if (UStaticMeshComponent* MeshComponent = SpawnedActor->FindComponentByClass<UStaticMeshComponent>())
            {
                if (MeshComponent->GetStaticMesh())
                {
                    UE_LOG(LogTemp, Log, TEXT("Actor has mesh: %s"), 
                        *MeshComponent->GetStaticMesh()->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Static mesh component exists but no mesh is assigned"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No static mesh component found on the spawned actor"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to spawn actor"));
        }
    }
}
