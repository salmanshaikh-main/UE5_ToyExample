// APIManager.cpp
#include "APIManager.h"

UAPIManager::UAPIManager()
{
    // Constructor
}

void UAPIManager::ActivateSuperpower()
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UAPIManager::OnActivateSuperpowerResponse);
    Request->SetURL("http://localhost:5000/activate_superpower");
    Request->SetVerb("GET");
    Request->ProcessRequest();
}

void UAPIManager::OnActivateSuperpowerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("Superpower activated: %s"), *Response->GetContentAsString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to activate superpower"));
    }
}