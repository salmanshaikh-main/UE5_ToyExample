// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class THIRDPERSON_API Logger
{
public:
	Logger();
	~Logger();

	void WriteMovementDataToJson(const FString& FilePath, const FVector& PlayerLocation, double TimeSeconds, uint64 FrameNumber);

};
