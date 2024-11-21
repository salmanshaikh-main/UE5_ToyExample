// CustomDepthActor.h
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomDepthActor.generated.h"

UCLASS()
class THIRDPERSON_API ACustomDepthActor : public AActor
{
    GENERATED_BODY()
public:
    ACustomDepthActor();

    UPROPERTY(EditDefaultsOnly, Category = "Rendering")
    bool bEnableCustomDepth = true;

    UPROPERTY(EditDefaultsOnly, Category = "Rendering")
    int32 CustomDepthStencilValue = 1;

    // Add a reference to the mesh component
    UPROPERTY(VisibleAnywhere, Category = "Mesh")
    UStaticMeshComponent* MeshComponent;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};