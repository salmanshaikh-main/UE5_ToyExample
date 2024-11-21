// CustomDepthActor.cpp
#include "CustomDepthActor.h"
#include "UObject/ConstructorHelpers.h"

ACustomDepthActor::ACustomDepthActor()
{
    // Create and setup the static mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Load and set a default static mesh (cube in this case)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (MeshAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(MeshAsset.Object);
        // Set a reasonable default size
        MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
    }

    // Enable CustomDepth pass by default
    if (MeshComponent)
    {
        MeshComponent->SetRenderCustomDepth(true);
        MeshComponent->SetCustomDepthStencilValue(1);
    }

    // Enable ticking
    PrimaryActorTick.bCanEverTick = true;
}

void ACustomDepthActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Update CustomDepth settings based on properties
    if (MeshComponent)
    {
        MeshComponent->SetRenderCustomDepth(bEnableCustomDepth);
        MeshComponent->SetCustomDepthStencilValue(CustomDepthStencilValue);
    }
}

void ACustomDepthActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}