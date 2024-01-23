// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogScenario, Log, All);


struct AutoMove 
{
	float timestamp;
	float duration;
	FVector Vector;
};

/**
 * 
 */
class THIRDPERSON_API UScenario
{
public:
	UScenario();

	~UScenario();

	void InitializeScenarioFromArgs();

	FVector GetMove(double Time);

private:
	TArray<AutoMove> Movements;


	void DeserializeJsonFile(const FString& FilePath);

	

};
