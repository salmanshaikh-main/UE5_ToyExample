// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ThirdPersonCharacter.h"

void ACustomPlayerController::Client_OnResponseReceived_Implementation(const FString& PlayerID, const FString& ResponseString)
{
    if (IsLocalController())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Player %s: %s"), *PlayerID, *ResponseString));
    }
}