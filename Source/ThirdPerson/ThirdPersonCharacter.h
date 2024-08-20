// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "UScenario.h"
#include "Weapon.h"
#include "GameFramework/PlayerState.h"
#include "APIManager.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "ThirdPersonCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AThirdPersonCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UScenario Scenario;

public:
	AThirdPersonCharacter();

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Getter for Max Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetCurrentHealth(float healthValue);

	UFUNCTION(BlueprintCallable, Category="Test")
	void SubmitButton(const FString& PlayerInput);

    // Server RPC to handle the input on the server
    UFUNCTION(Server, Reliable)
    void ServerHandlePlayerInput(const FString& PlayerInput);

	// Client RPC to notify the player
    UFUNCTION(Client, Reliable)
    void ClientNotifyValidInput(const FString& Message);

	UFUNCTION(Client, Reliable)
	void TargetedClientReceiveMessage(const FString& Message);

	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;

    UFUNCTION(Server, Reliable)
    void ServerRespawn(FVector RespawnLocation);

	//UFUNCTION(Server, Reliable)
	//void ServerPlayerDied();
    // Hit-scan weapon properties
    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Weapon")
    float HitScanRange = 10000.f;

    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Weapon")
    float HitScanDamage = 5.f;

    // Function to fire the hit-scan weapon
    UFUNCTION(BlueprintCallable, Category="Gameplay")
    void FireHitScanWeapon();

    // Server RPC for hit-scan weapon damage validation
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerValidateHitScanDamage(AActor* HitActor, FVector_NetQuantize HitLocation);

	UFUNCTION(Client, Reliable)
	void Client_SetPacketLoss(int OutLossRate, int InLossRate);


protected:
	/** Called every tick, used for auto movement */
	virtual void Tick(float DeltaTime);

	/** Get the movement scenario from Cli args */
	void GetMovementScenario();

	void WriteMovementDataToJson(const FString& FilePath, const FVector& PlayerLocation, double Time, uint64 FrameNumber, long long int TimeSec, long long int TimeNano);

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

	/** Movement to a specific location */
	void MoveToPosition(FVector TargetLocation);

	//void RestartLevelOnServer();

	/** The player's maximum health. This is the highest value of their health can be. This value is a value of the player's health, which starts at when spawned.*/
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	/** The player's current health. When reduced to 0, they are considered dead.*/
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	/** RepNotify for changes made to current health.*/
	UFUNCTION()
	void OnRep_CurrentHealth();

	/** Response to health being updated. Called on the server immediately after modification, and on clients in response to a RepNotify*/
	void OnHealthUpdate();

	/** Function to be invoked on the client's machine when their health drops to 0*/
	void OnDeath();

	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Projectile")
	TSubclassOf<class AThirdPersonProjectile> ProjectileClass;

	/** Delay between shots in seconds. Used to control fire rate for your test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.*/
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	float FireRate;

	/** If true, you are in the process of firing projectiles. */
	bool bIsFiringWeapon;

	/** Function for beginning weapon fire.*/
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StartFire();

	/** Function for ending weapon fire. Once this is called, the player can use StartFire again.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();

	/** Server function for spawning projectiles.*/
	UFUNCTION(Server, Reliable)
	void HandleFire(const FVector& spawnLocation, const FRotator& spawnRotation);

	UFUNCTION(BlueprintCallable, Category = "API")
	void APIRequest();

	UFUNCTION(Server, Reliable)
	void ServerCallAPI(const FString& PlayerID);

// ----------------------------------------------------------------

	// UFUNCTION(BlueprintCallable, Category = "SuperpowerClient")
	// bool IsPythonInstalled();

	// UFUNCTION(BlueprintCallable, Category = "SuperpowerClient")
	// void ExecuteScript();
	UFUNCTION(BlueprintCallable, Category = "Superpower")
	void CallRunningService();

	void OnServiceResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	UFUNCTION(BlueprintCallable, Category = "Superpower")
    void ActivateSuperpower();
	

	/** A timer handle used for providing the fire rate delay in-between spawns.*/
	FTimerHandle FiringTimer;

	void LogMovement();
	FString GetPlayerIdentifier() const;

	// Debug line draw duration
    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Debug")
    float DebugDrawDuration = 0.15f;

	//dossss
	UFUNCTION()
	void ClientInvokeRPC();

	// Function to request player IDs from the server
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Superpower")
	void GetIds();

	UFUNCTION(Client, Reliable)
	void PlayerRecIds(const FString& Ids);


public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	UPROPERTY()
    UAPIManager* APIManager;
};


