// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActPlg.generated.h"

UCLASS()
class THIRDPERSON_API AMyActPlg : public AActor
{
	GENERATED_BODY()
	
public:	
	AMyActPlg();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
