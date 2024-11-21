// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "UScenario.h"
#include "GameFramework/PlayerState.h"
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

	// Function to check if character is within the defined zone
    bool IsCharacterInZone(const FVector& CharacterLocation);

	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;
	
	/** Getter for Max Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category="Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category="Health")
	void SetCurrentHealth(float healthValue);

	UFUNCTION()
	void SetCurrentHealthWrapper();

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Denial of Service */ 

	// Function that initates receving of other connected players' IDs
	UFUNCTION()
	void ClientInvokeRPC();

	UFUNCTION()
	void SetPacketLossForMe();

	// Server RPC to get the player IDs
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Superpower")
	void GetIds();

	// Client RPC to receive the player IDs
	UFUNCTION(Client, Reliable)
	void PlayerRecIds(const FString& Ids);

	// On clicking Submit (after opening UI to input player's tag or IP).
	UFUNCTION(BlueprintCallable, Category="Test")
	void SubmitButton(const FString& PlayerInput);

    // Server RPC to handle the input on the server.
    UFUNCTION(Server, Reliable)
    void ServerHandlePlayerInput(const FString& PlayerInput);

	// Client RPC to notify the player.
    UFUNCTION(Client, Reliable)
    void ClientNotifyValidInput(const FString& Message);

	// Setting packet losses at targeted victim.
	UFUNCTION(Client, Reliable)
	void Client_SetPacketLoss(int OutLossRate, int InLossRate);

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Lag Switch and Fixed Delay */ 

	UFUNCTION()
	void ClientInvokeLS();
	UFUNCTION()
	void ClientInvokeFD();

	// Performing a lag switch 
	UFUNCTION()
	void LagSwitchFunc();

	// Adding artificial delay to the player's outgoing packets
	UFUNCTION()
	void FixedDelayFunc();

	// Go back to normal behaviour after specified time
	UFUNCTION()
	void RevertArtLag();

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Aimbot */ 

	// Start aimbot
	UFUNCTION()
	void Aimbot();

	// Start looking for opponents in the field of view
	UFUNCTION()
	void AimbotTick();

	// Find closest enemy in the field of view
	UFUNCTION()
	void FindTargetInFOV();

	// Change camera configuration to aim at the target
	UFUNCTION()
    void AimAtTarget();

	// Stop aimbot after a specified duration
	UFUNCTION()
    void StopAimbot();

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Attacks residing outside of the game */
	
	// Pressing the button to start the attack (communicate with a local API)
	UFUNCTION(BlueprintCallable, Category = "Superpower")
	void CallRunningService();

	// The API running locally, performs some action and returns a response
	void OnServiceResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
	/////////////////////////////////////////////////////////////////////////////////////////
	/** Logging */

	// Variable for remembering player set interval
	double PlayerClientSetInterval;
	double PlayerServerSetInterval;

	// Initial UI button pressed by client to set type of logging
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void SubmitNonePreference();
	double None_Client;
	double None_Server; 
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void SubmitModeratePreference();
	double Moderate_Client;
	double Moderate_Server;
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void SubmitExtensivePreference();
	double Extensive_Client;
	double Extensive_Server;

	// Periodic Server log caller
	UFUNCTION(Server, Reliable)
	void LogCallerServer(double ServerInterval);

	// Periodic client log caller
	UFUNCTION()
	void LogCallerClient(double ClientInterval);

	// Set the paths for the log files (Client and Server)
	UFUNCTION()
	void SetLogPaths();

	// Set the player's name for logging at server and client
	UPROPERTY() 
	FString PlayerNameForServerLog;
	UPROPERTY() 
	FString PlayerNameForClientLog;

	// Check if the log paths have been set (called at first tick)
	bool bHasSetLogPaths = false;

	// Timerhandle for client and server logging
	FTimerHandle ClientTimerHandle;
	FTimerHandle ServerTimerHandle;

	// Main logger function
	UFUNCTION()
	void MainLogger(bool Shot = false);

	UFUNCTION()
	void MainLoggerWrapper();


	// Sending client data to server
    double TimeSinceLastSend;

	// String used to store the path of the log file at the server and client
	FString MainServerPath;
	FString MainClientPath;

	// Get the player's identifier
	FString GetPlayerIdentifier() const;

	// Send the log data to the server at particular time period 
	UFUNCTION()
	void SendClientLogData();

	// Receive the log data from the client
	UFUNCTION(Server, Reliable)
    void ServerReceiveLogData(const FString& LogData);

	// Reset log intervals 
	UFUNCTION()
	void ResetLogIntervals();

	// TimerHandle to clear and set each time mainlogger is called
	FTimerHandle TimerHandleLog;

	/////////////////////////////////////////////////////////////////////////////////////////

protected:
	/** Called every tick, used for auto movement */
	virtual void Tick(float DeltaTime);

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

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

	/** Specifies the class of projectile to spawn when firing. */
	UPROPERTY(EditDefaultsOnly, Category="Gameplay|Projectile")
	TSubclassOf<class AThirdPersonProjectile> ProjectileClass;

	/** If true, you are in the process of firing projectiles. */
	bool bIsFiringWeapon;

	/** A timer handle used for providing the fire rate delay in-between spawns.*/
	FTimerHandle FiringTimer;

	/** Delay between shots in seconds. Used to control fire rate for your test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.*/
	UPROPERTY(EditDefaultsOnly, Category="Gameplay")
	float FireRate;

	/** Function for beginning weapon fire.*/
	UFUNCTION(BlueprintCallable, Category="Gameplay")
	void StartFire();

	/** Function for ending weapon fire. Once this is called, the player can use StartFire again.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();

	/** Server function for spawning projectiles.*/
	UFUNCTION(Server, Reliable)
	void HandleFire(const FVector& spawnLocation, const FRotator& spawnRotation);

	/** HitScan weapon fire function.*/
    UFUNCTION(BlueprintCallable, Category="Gameplay")
    void FireHitScanWeapon();

	/** Debug line draw duration */ 
    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Debug")
    float DebugDrawDuration = 0.15f;

	/** Weapon range */
    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Weapon")
    float HitScanRange = 10000.f;

	/** Weapon damage */
    UPROPERTY(EditDefaultsOnly, Category="Gameplay|Weapon")
    float HitScanDamage = 5.f;

	/** Damage applied on server if actor is ThirdPersonCharacter */
    UFUNCTION(Server, Reliable)
    void ServerValidateHitScanDamage(AActor* HitActor, FVector_NetQuantize HitLocation);	

	/** Relocate and respawn the player after health hits 0. */
	UFUNCTION(Server, Reliable)
    void ServerRespawn(FVector RespawnLocation);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
	/** Movement to a specific location */
	void MoveToPosition(FVector TargetLocation);

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Variables to be used throughout the duration of client's instance */

	// Boolean to check if any of the attacks are active (all false by default)
	bool ArtDelay = false;
	bool LagSwitch = false;
	bool RecIDs = false;
	bool DoS = false;
	bool AimBot = false;

	// Finding the spawn time of the character at the server instance of the game
	double SpawnTime;

	/////////////////////////////////////////////////////////////////////////////////////////
	/** Aimbot Configuration  */

	float TargetAcquiredTime = 0.0f;

	// Set the rotation of the character on client side
	UFUNCTION()
	void SetRotation(FRotator NewRotation);

	// Set the rotation of the character on server's instance
	UFUNCTION(Server, Unreliable)
	void ServerSetRotation(FRotator NewRotation);

	// Set the rotation of the character on all clients
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastSetRotation(FRotator NewRotation);

	// Replicated variable to store the character's rotation
    UPROPERTY(ReplicatedUsing=OnRep_ReplicatedRotation)
    FRotator ReplicatedRotation;

	// Replicate rotation to other clients
	UFUNCTION()
    void OnRep_ReplicatedRotation();

	// Other players (used for Aimbot)
	AThirdPersonCharacter* ClosestEnemy;

	// Self player
    AThirdPersonCharacter* CurrentTarget;

	// Timer handles for Aimbot
	bool bAimbotActive = false;
	
    FTimerHandle AimbotTickTimerHandle;
    FTimerHandle AimbotStopTimerHandle;

	// Field of View where aimbot will activate
    float AimbotFOV = 45.0f;

	// Aimbot distance limit
    float AimbotMaxDistance = 10000.0f;

	// Duration in seconds before auto-stop
    float AimbotDuration = 5.0f; 
	    
	bool bHasSpawnedObject = false;
    
    // Optional: Store the time threshold as a variable
    const float SpawnTimeThreshold = 10.0f;
};