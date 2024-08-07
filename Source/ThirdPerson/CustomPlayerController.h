// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CustomPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSON_API ACustomPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    UFUNCTION(Client, Reliable)
    void Client_OnResponseReceived(const FString& PlayerID, const FString& ResponseString);

};
