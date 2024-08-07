// APIManager.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "APIManager.generated.h"

UCLASS(Blueprintable, BlueprintType)
class THIRDPERSON_API UAPIManager : public UObject
{
    GENERATED_BODY()

public:
    UAPIManager();

    UFUNCTION(BlueprintCallable, Category = "API")
    void ActivateSuperpower();

private:
    void OnActivateSuperpowerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
