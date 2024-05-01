// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "Weapon.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Log, All);

UCLASS()
class THIRDPERSON_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon Actions")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon Actions")
	void StopFire();

protected:
	virtual void BeginPlay() override;

	void FireShot();

	FTimerHandle TimerHandle_HandleRefire;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	// the seconds to wait between shots 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons Properties")
	float TimeBetweenShots;

	// set true to enable the Fire animation related to the weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons Properties")
	bool FireAnimation;

	// The length of the hitscan trace
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float TraceDistance;


	// dropping weapon not implemented; should add a sphere for that 

};
