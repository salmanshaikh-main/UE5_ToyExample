// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

DEFINE_LOG_CATEGORY(LogWeapon);


AWeapon::AWeapon()
	:
	TimeBetweenShots(0.15f),
	FireAnimation(true),
	TraceDistance(10000.f)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	SetRootComponent(WeaponMesh);

}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

}

void AWeapon::StartFire()
{
	FireShot();

	GetWorldTimerManager().SetTimer(TimerHandle_HandleRefire, this, &AWeapon::FireShot, TimeBetweenShots, true);

}

void AWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_HandleRefire);
}

void AWeapon::FireShot()
{
	UE_LOG(LogWeapon, Log, TEXT("Shooting HItscan weapon"));


	FHitResult HitResult;
	FCollisionQueryParams TraceParams(FName(TEXT("WeaponTrace")), true, GetOwner());

	FVector Start = GetActorLocation();
	FVector End = Start + GetActorForwardVector() * TraceDistance;


	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, TraceParams))
	{
		// Hit something
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// Handle the hit actor
			// For example, you might apply damage here
			// Or trigger some effects
		}

		DrawDebugLine(GetWorld(), Start, HitResult.ImpactPoint, FColor::Green, false, 1, 0, 1);
	}
	else
	{
		// Draw a debug line to visualize the trace
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 1, 0, 1);
	}



	//TODO: add a Fire Montage + sound
	// see which camera use to create the trace
	// const FVector StartTrace = 

	// see how to imlpement the Fire Animation
	// play a FireAnimation if specified
	//if (FireAnimation != NULL)
	//{
	//	UAnimInstance* AnimInstance = WeaponMesh->GetAnimInstance();
	//	if (AnimInstance != NULL)
	//	{
	//		AnimInstance->Montage_Play(FireAnimation, 1.f);
	//	}
	//}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

