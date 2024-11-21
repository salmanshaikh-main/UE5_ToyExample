#include "ThirdPersonCharacter.h"
#include "ThirdPersonGameMode.h"
#include "ThirdPersonProjectile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameSession.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "DrawDebugHelpers.h"
#include "HAL/PlatformProcess.h"
#include "Blueprint/UserWidget.h"  
#include "Kismet/GameplayStatics.h"  
#include "Sockets.h"
#include "IPAddress.h"
#include "Networking.h"
#include "EngineUtils.h"
#include <iostream>
#include <fstream> 
#include <ctime>
#include <chrono>


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

/////////////////////////////////////////////////////////////////////////////////////////////////
// AThirdPersonCharacter

AThirdPersonCharacter::AThirdPersonCharacter()
{

    None_Client = -1.0;
    None_Server = -1.0;
    Moderate_Client = -1.0;
    Moderate_Server = -1.0;
    Extensive_Client = -1.0;
    Extensive_Server = -1.0;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	//Initialize the player's Health
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;

	//Initialize projectile class
    ProjectileClass = AThirdPersonProjectile::StaticClass();

    //Initialize fire rate
    FireRate = 0.25f;
    bIsFiringWeapon = false;
}

void AThirdPersonCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (HasAuthority())
	{
		// Capture the server time at which the character was spawned
		SpawnTime = GetWorld()->GetTimeSeconds();
	}

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// As of now, no close enemy (aimbot config)
	ClosestEnemy = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AThirdPersonCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AThirdPersonCharacter::Look);

		// Handle firing projectiles
    	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AThirdPersonCharacter::StartFire);

		// Handle hit scan fires
		PlayerInputComponent->BindAction("FireHitScan", IE_Pressed, this, &AThirdPersonCharacter::FireHitScanWeapon);

		// Handle getting IDs of connected players
		PlayerInputComponent->BindAction("DoS", IE_Pressed, this, &AThirdPersonCharacter::ClientInvokeRPC);
		// Invoking of UI after getting IDs is done by another button specified in blueprints and widgets

        PlayerInputComponent->BindAction("selfdos", IE_Pressed, this, &AThirdPersonCharacter::SetPacketLossForMe);

		// Handle setting Lag Switch configuration and performing it
		PlayerInputComponent->BindAction("lagswitch", IE_Pressed, this, &AThirdPersonCharacter::ClientInvokeLS);

		// Handle setting Fixed Delay configuration and performing it
		PlayerInputComponent->BindAction("fixeddelay", IE_Pressed, this, &AThirdPersonCharacter::ClientInvokeFD);

	    // Handle starting of Aimbot
		PlayerInputComponent->BindAction("Aimbot", IE_Pressed, this, &AThirdPersonCharacter::Aimbot);

		// Handle calling of local service running in the background (if set up)
		PlayerInputComponent->BindAction("CallService", IE_Pressed, this, &AThirdPersonCharacter::CallRunningService);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

// Replicated Properties
void AThirdPersonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(AThirdPersonCharacter, CurrentHealth);
	//Replicate current rotation. (activation of aimbot)
	DOREPLIFETIME(AThirdPersonCharacter, ReplicatedRotation);
}

void AThirdPersonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
	// Call SetLogPaths() only once
	if (!bHasSetLogPaths)
	{
		SetLogPaths();
		bHasSetLogPaths = true;
		if (HasAuthority()) 
		{
			// if this tag present in the command line, then set the server log path
			if (FParse::Param(FCommandLine::Get(), TEXT("mainServerLog"))) 
			{
				MainServerPath = FPaths::Combine(TEXT("./Logs/"), *PlayerNameForServerLog + FString(TEXT(".json")));
			}
		}
		else 
		{
			// if this tag present in the command line, then set the client log path
			if (FParse::Param(FCommandLine::Get(), TEXT("mainClientLog"))) {
				MainClientPath = FPaths::Combine(TEXT("./Logs/"), *PlayerNameForClientLog + FString(TEXT(".json")));
			}
		}
	}

	// Get current world
	UWorld* World = GetWorld();

	// Get current player location
	FVector PlayerLocation = GetActorLocation();

	// Print location string to screen for debugging
	FString LocationString = FString::Printf(TEXT("Current Location: X=%.2f Y=%.2f Z=%.2f"), PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z);
    GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, LocationString);
    //UE_LOG(LogTemplateCharacter, Log, TEXT("Time: %f | Current Location: X=%.2f Y=%.2f Z=%.2f"), DeltaTime,PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z);

    //////////////////////////////////////////////////////////////////////////
	/** JSON file automated movements */
	double timeSeconds = World->GetTimeSeconds();
	FVector Vector;
	Vector = Scenario.GetMove(timeSeconds, PlayerLocation);

    if (!bHasSpawnedObject)  // Only check if we haven't spawned yet
    {
        
        if (timeSeconds >= SpawnTimeThreshold)
        {
            if (AThirdPersonGameMode* GameMode = Cast<AThirdPersonGameMode>(GetWorld()->GetAuthGameMode()))
            {
                GameMode->SpawnObjectForRandomPlayer();
                bHasSpawnedObject = true;  // Set flag to true after spawning
                UE_LOG(LogTemp, Log, TEXT("Spawned object at %f seconds"), timeSeconds);
            }
        }
    }
	
	// If vector is zero, send one last move to the last position
	if (!Vector.IsZero())
	{
		MoveToPosition(Vector);
	}
    //////////////////////////////////////////////////////////////////////////

    TimeSinceLastSend += DeltaTime;


    if(TimeSinceLastSend >= 60.0f)
    {
        UE_LOG(LogTemplateCharacter, Log, TEXT("Sending client log data"));
        SendClientLogData();
        TimeSinceLastSend = 0.0f; 
    }
}

// Handles character movement based on player input
void AThirdPersonCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out  which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

	}
}

// Handles character rotation based on player input
void AThirdPersonCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

// Function to move the character to a specific position
void AThirdPersonCharacter::MoveToPosition(FVector TargetLocation)
{
	// Find the difference between targetlocation and current location
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Normalize();
	// Move the character to the targeted location
	AddMovementInput(Direction);
	UE_LOG(LogTemplateCharacter, Log, TEXT("Auto Move to position: %f %f %f"), Direction.X, Direction.Y, Direction.Z);
}

// Client initiates process to recieve player IDs
void AThirdPersonCharacter::ClientInvokeRPC()
{
	GetIds();
}

// Server-side function implementation to get player IDs
void AThirdPersonCharacter::GetIds_Implementation()
{
    if (HasAuthority()) 
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        TArray<TSharedPtr<FJsonValue>> PlayersArray;

        // Get the player controller of the invoking client
        APlayerController* InvokingPlayerController = Cast<APlayerController>(GetController());
        bool bOtherPlayersFound = false;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
			// Exclude the invoking player controller
            if (PC && PC != InvokingPlayerController) 
            {
                UNetConnection* NetConnection = Cast<UNetConnection>(PC->Player);
                if (NetConnection)
                {
                    bOtherPlayersFound = true;
                    TSharedPtr<FJsonObject> PlayerInfo = MakeShareable(new FJsonObject());
                    FString IPAddressString = NetConnection->GetRemoteAddr()->ToString(false);
                    FString PlayerID = PC->PlayerState ? PC->PlayerState->GetPlayerName() : FString("Unknown");
                    PlayerInfo->SetStringField("IPAddress", IPAddressString);
                    PlayerInfo->SetStringField("PlayerID", PlayerID);
                    PlayersArray.Add(MakeShareable(new FJsonValueObject(PlayerInfo)));
                }
            }
        }
        if (bOtherPlayersFound)
        {
            JsonObject->SetArrayField("Players", PlayersArray);
        }
        else
        {
            JsonObject->SetStringField("Message", "No other players connected");
        }
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        PlayerRecIds(OutputString); // Send formatted JSON to the client
    }
}

// Client-side function to receive player IDs
void AThirdPersonCharacter::PlayerRecIds_Implementation(const FString& JsonString)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        FString MessageToDisplay;
        const TArray<TSharedPtr<FJsonValue>>* PlayersArray;
        if (JsonObject->TryGetArrayField("Players", PlayersArray))
        {
            for (const TSharedPtr<FJsonValue>& PlayerValue : *PlayersArray)
            {
                TSharedPtr<FJsonObject> PlayerInfo = PlayerValue->AsObject();
                FString IPAddress = PlayerInfo->GetStringField("IPAddress");
                FString PlayerID = PlayerInfo->GetStringField("PlayerID");
                MessageToDisplay += FString::Printf(TEXT("Player IP Address: %s, Player ID: %s\n"), *IPAddress, *PlayerID);
            }
        }
        else
        {
            FString Message;
            if (JsonObject->TryGetStringField("Message", Message))
            {
                MessageToDisplay = Message;
            }
        }
        // Display the message on the screen
        if (!MessageToDisplay.IsEmpty())
        {
            GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Blue, MessageToDisplay);
        }
    }
}

// After user presses a button to open a UI, they input some ID and click on the submit button, which executes this function
void AThirdPersonCharacter::SubmitButton(const FString& PlayerInput)
{
    ServerHandlePlayerInput(PlayerInput);
}

// Server-side function to handle player input
void AThirdPersonCharacter::ServerHandlePlayerInput_Implementation(const FString& PlayerInput)
{
    // Perform IP address validation
    FIPv4Address IPAddress;
    if (!FIPv4Address::Parse(PlayerInput, IPAddress))
    {
        ClientNotifyValidInput(FString("Invalid IP address format!"));
        return;
    }

    // Iterate through all connected players except the one invoking call
	APlayerController* TargetedPlayerController = nullptr;
	APlayerController* InvokingPlayerController = Cast<APlayerController>(GetController());
	FString InvokingPlayerName = InvokingPlayerController ? InvokingPlayerController->PlayerState->GetPlayerName() : FString();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		// Exclude the invoking player controller
		if (PC && PC != InvokingPlayerController)  
		{
			UNetConnection* NetConnection = Cast<UNetConnection>(PC->Player);
			if (NetConnection)
			{
				FString ConnectedIPAddress = NetConnection->GetRemoteAddr()->ToString(false);

				// Compare the input IP address with the connected IP address
				if (ConnectedIPAddress.Equals(PlayerInput))
				{
					FString PlayerName = PC->PlayerState->GetPlayerName();
					// Exclude player by name
					if (!PlayerName.Equals(InvokingPlayerName))  
					{
						TargetedPlayerController = PC;  
						break;
					}
				}
			}
		}
	}

	// if a controller is found to be the same as received player ID
	if (TargetedPlayerController)
	{        
		AThirdPersonCharacter* TargetedCharacter = Cast<AThirdPersonCharacter>(TargetedPlayerController->GetPawn());
		if (TargetedCharacter)
		{
			AThirdPersonGameMode* GameMode = Cast<AThirdPersonGameMode>(GetWorld()->GetAuthGameMode());
			if (GameMode)
			{
				// Perform the DoS attack
                //LogCallerClient(0.05f);
                //LogCallerServer(0.05f);

				double duration = 5.0f;
				GameMode->SimulateDoSAttack(TargetedPlayerController, 85, 95, duration);
				//DoS = true;

				//Change server and client logging to be more rapid for the duration of the DoS attack
				
			}
			else
			{
				ClientNotifyValidInput(FString("Failed to perform DoS on player: GameMode not found or is not AThirdPersonGameMode."));
			}
		}
		else
		{
			ClientNotifyValidInput(FString("Found player controller, but couldn't get character!"));
		}
	}
}

// Function that the server invokes on the targeted client 
void AThirdPersonCharacter::Client_SetPacketLoss_Implementation(int InLossRate, int OutLossRate)
{
	if (OutLossRate != 0 || InLossRate != 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Denial of Service atttack launched on you!")));
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Packet loss rates set to %d%% (outgoing) and %d%% (incoming)"), OutLossRate, InLossRate));
    
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        // Command to set incoming packet loss
        FString Command2 = FString::Printf(TEXT("NetEmulation.PktIncomingLoss %d"), InLossRate);
        PC->ConsoleCommand(*Command2);

        // Command to set outgoing packet loss
        FString Command = FString::Printf(TEXT("NetEmulation.PktLoss %d"), OutLossRate);
        PC->ConsoleCommand(*Command);

        DoS = true;
        if (OutLossRate == 0 || InLossRate == 0)
        {
             GetWorld()->GetTimerManager().ClearTimer(TimerHandleLog);
             ResetLogIntervals();
             DoS = false;
         }
         GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Packet loss rates set to %d%% (outgoing) and %d%% (incoming)"), OutLossRate, InLossRate));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to find PlayerController for setting packet loss"));
    }
}

void AThirdPersonCharacter::SetPacketLossForMe()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        // Command to set outgoing packet loss
        FString Command = FString::Printf(TEXT("NetEmulation.PktLoss 75"));
        PC->ConsoleCommand(*Command);

        // Command to set incoming packet loss
        FString Command2 = FString::Printf(TEXT("NetEmulation.PktIncomingLoss 75"));
        PC->ConsoleCommand(*Command2);
    }

    if(HasAuthority())
    {
        FTimerHandle HealthHandle;
        GetWorld()->GetTimerManager().SetTimer(HealthHandle, this, &AThirdPersonCharacter::SetCurrentHealthWrapper, 2.0f, false);
    }

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AThirdPersonCharacter::RevertArtLag, 15.0f, false);
}

void AThirdPersonCharacter::SetCurrentHealthWrapper()
{
    if(HasAuthority())
    {
        UE_LOG(LogTemplateCharacter, Log, TEXT("Setting health to 80.0f"));
        SetCurrentHealth(80.0f);
    }
}

// Invoking client receives the message from the server whether or not the given player ID is valid or not
void AThirdPersonCharacter::ClientNotifyValidInput_Implementation(const FString& Message)
{
    if (Message == "Player with IP address found, message sent!" || Message == "Player with IP address found!" || Message == "Player with IP address found! Applied DoS attack!")
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Message);
    }
}

void AThirdPersonCharacter::ClientInvokeLS()
{
    LagSwitch = true;
    
    LogCallerClient(0.5f);
    LogCallerServer(0.5f);

    FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AThirdPersonCharacter::LagSwitchFunc, 0.1f, false);
}
void AThirdPersonCharacter::ClientInvokeFD()
{
    ArtDelay = true;

    //LogCallerClient(0.5f);
    //LogCallerServer(0.5f);
    
    FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AThirdPersonCharacter::FixedDelayFunc, 0.01f, false);
}

// Client side function to invoke Lag Switch attack 
void AThirdPersonCharacter::LagSwitchFunc()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Console command to add delay
        FString Command = FString::Printf(TEXT("NetEmulation.PktLag 1500"));
        PC->ConsoleCommand(*Command);

		// Set a timer to revert the lag switch before server starts receiving the delayed packets
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AThirdPersonCharacter::RevertArtLag, 1.20f, false);
    }
}

// Client side function to invoke Fixed Delay attack
void AThirdPersonCharacter::FixedDelayFunc()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Console command to add delay
        FString Command = FString::Printf(TEXT("NetEmulation.PktLag 60000"));
        PC->ConsoleCommand(*Command);

		// Set a timer to revert the delay to normal transmission of packets
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AThirdPersonCharacter::RevertArtLag, 66.0f, false);
    }
}

// Revert the Lag Switch attack and Fixed Delay attack
void AThirdPersonCharacter::RevertArtLag()
{
	LagSwitch = false;
	ArtDelay = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Console command to turn off net emulation
		FString Command = FString::Printf(TEXT("NetEmulation.Off"));
		PC->ConsoleCommand(*Command);
	}

    GetWorld()->GetTimerManager().ClearTimer(TimerHandleLog);
    ResetLogIntervals();
}

// Update health if damaged 
void AThirdPersonCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void AThirdPersonCharacter::OnHealthUpdate()
{
	//Client-specific functionality
	if (IsLocallyControlled())
	{
        MainLogger(true);
		if (CurrentHealth <= 0)
		{
			// Handle player "death"
			FVector RespawnLocation = FVector(1950.00, 30, 322.15);
			ServerRespawn(RespawnLocation);
			SetActorRotation(FRotator(180, 0, 0));
			SetActorRotation(FRotator(-90, 0, 0));
		}
	}

	//Server-specific functionality
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		UE_LOG(LogTemplateCharacter, Log, TEXT("%s"), *healthMessage);
	}
}

void AThirdPersonCharacter::ServerRespawn_Implementation(FVector NewLocation)
{
    if (HasAuthority())
    {
        // Stop any ongoing movement
        GetCharacterMovement()->StopMovementImmediately();

		// Set new location
        SetActorLocation(NewLocation);

		// Reset health
        CurrentHealth = MaxHealth;

		// Reset rotation
        SetActorRotation(FRotator(-180, 0, 0));

        // Notify clients that the character has respawned
        OnRep_CurrentHealth();

		FString respawnMessage = FString::Printf(TEXT("%s has now respawned with full health."), *GetFName().ToString());
		UE_LOG(LogTemplateCharacter, Log, TEXT("%s"), *respawnMessage);
    }
}

// Function to set the current health of the character
void AThirdPersonCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

// Function to handle the damage taken by the character
float AThirdPersonCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}

// Projectile weapon firing config
void AThirdPersonCharacter::StartFire()
    {
        if (!bIsFiringWeapon)
        {
            bIsFiringWeapon = true;
            UWorld* World = GetWorld();
            World->GetTimerManager().SetTimer(FiringTimer, this, &AThirdPersonCharacter::StopFire, FireRate, false);
            FVector spawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 200.0f) + (GetActorUpVector() * 62.0f);
        	FRotator spawnRotation = GetActorRotation();
        	spawnRotation.Yaw -= 2.0f;
			HandleFire(spawnLocation, spawnRotation);
        }
    }

// Stop firing the projectile weapon
void AThirdPersonCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

// Function to handle the firing of the projectile (server side)
void AThirdPersonCharacter::HandleFire_Implementation(const FVector& spawnLocation, const FRotator& spawnRotation)
{
	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AThirdPersonProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonProjectile>(spawnLocation, spawnRotation, spawnParameters);
    UE_LOG(LogTemplateCharacter, Log, TEXT("Projectile fired!"));
    MainLogger(true);
}

// Function to handle the firing of the hitscan weapon (client side)
void AThirdPersonCharacter::FireHitScanWeapon()
{
    FVector SpawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 90.0f) + (GetActorUpVector() * 62.0f);
    FRotator SpawnRotation = GetActorRotation();
    SpawnRotation.Yaw -= 2.0f;

    FVector Start = SpawnLocation;
    FVector End = Start + (SpawnRotation.Vector() * HitScanRange);

    //MainLogger(true);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);

    if (bHit)
    {
        // Draw green line from start to hit point
        DrawDebugLine(GetWorld(), Start, HitResult.ImpactPoint, FColor::Green, false, DebugDrawDuration, 0, 2.0f);
        
        // Draw red line from hit point to end of range
        DrawDebugLine(GetWorld(), HitResult.ImpactPoint, End, FColor::Red, false, DebugDrawDuration, 0, 2.0f);
        
        // Draw hit point
        DrawDebugPoint(GetWorld(), HitResult.ImpactPoint, 20.f, FColor::Red, false, DebugDrawDuration);

        AActor* HitActor = HitResult.GetActor();

        // if hits an actor, check if actor is damageable
        if (HitActor)
        {
             ServerValidateHitScanDamage(HitActor, HitResult.ImpactPoint);
        }
    }
    else
    {
        // If no hit, draw entire line as green
        DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, DebugDrawDuration, 0, 2.0f);
    }
}

// Apply damage to the hit actor
void AThirdPersonCharacter::ServerValidateHitScanDamage_Implementation(AActor* HitActor, FVector_NetQuantize HitLocation)
{
    // This function runs on the server
    if (HitActor && HitActor->CanBeDamaged())
    {
        // Perform additional server-side validation here if needed
        // (e.g., check distance, line of sight, etc.)

        // Apply damage
        UGameplayStatics::ApplyPointDamage(HitActor, HitScanDamage, (HitLocation - GetActorLocation()).GetSafeNormal(), FHitResult(), GetInstigatorController(), this, UDamageType::StaticClass());
        MainLogger(true);
    }
}

void AThirdPersonCharacter::Aimbot()
{
    bAimbotActive = true;
    GetWorldTimerManager().SetTimer(AimbotTickTimerHandle, this, &AThirdPersonCharacter::AimbotTick, 0.016f, true);
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Aimbot started. Searching for targets..."));
}

void AThirdPersonCharacter::StopAimbot()
{
    bAimbotActive = false;
    GetWorldTimerManager().ClearTimer(AimbotTickTimerHandle);
    CurrentTarget = nullptr;
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Aimbot stopped."));
}

void AThirdPersonCharacter::AimbotTick()
{
    if (!bAimbotActive)
        return;

    FindTargetInFOV();
    if (CurrentTarget)
    {
        AimAtTarget();
        StopAimbot();
    }
}

// Find the target in the field of view
void AThirdPersonCharacter::FindTargetInFOV()
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Finding target in FOV!"));
    CurrentTarget = nullptr;
    float ClosestDistSq = FLT_MAX;
    
    FVector CameraLocation;
    FRotator CameraRotation;
    GetActorEyesViewPoint(CameraLocation, CameraRotation);
    
    FVector ForwardVector = CameraRotation.Vector();

    for (TActorIterator<AThirdPersonCharacter> It(GetWorld()); It; ++It)
    {
        AThirdPersonCharacter* PotentialTarget = *It;
        if (PotentialTarget == this || !IsValid(PotentialTarget))
            continue;

        TargetAcquiredTime = FPlatformTime::Seconds();
        FVector DirectionToTarget = (PotentialTarget->GetActorLocation() - CameraLocation).GetSafeNormal();
        float AngleToTarget = FMath::Acos(FVector::DotProduct(ForwardVector, DirectionToTarget));
        float AngleInDegrees = FMath::RadiansToDegrees(AngleToTarget);

        if (AngleInDegrees <= AimbotFOV / 2.0f)
        {
            FHitResult HitResult;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(this);

            if (GetWorld()->LineTraceSingleByChannel(HitResult, CameraLocation, PotentialTarget->GetActorLocation(), ECC_Visibility, Params))
            {
                if (HitResult.GetActor() == PotentialTarget)
                {
                    float DistSq = FVector::DistSquared(CameraLocation, PotentialTarget->GetActorLocation());
                    if (DistSq < ClosestDistSq && DistSq <= AimbotMaxDistance * AimbotMaxDistance)
                    {
                        CurrentTarget = PotentialTarget;
                        ClosestDistSq = DistSq;
                    }
                }
            }
        }
    }
}

// Aim at the target
void AThirdPersonCharacter::AimAtTarget()
{
    if (!CurrentTarget)
        return;

    FVector CameraLocation;
    FRotator CameraRotation;
    GetActorEyesViewPoint(CameraLocation, CameraRotation);

    FVector TargetLocation = CurrentTarget->GetActorLocation();
    FVector AimDirection = (TargetLocation - CameraLocation).GetSafeNormal();
    FRotator AimRotation = AimDirection.Rotation();

    // Set the new rotation immediately (snap)
    Controller->SetControlRotation(AimRotation);

    // Calculate the time it took to snap to the target
    float SnapTime = FPlatformTime::Seconds() - TargetAcquiredTime;

    // Log the snap time on screen
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Target found and snapped. Time to snap: %.4f seconds"), SnapTime));

    // Log the snap time to file
    FString FilePath = FPaths::ProjectSavedDir() + TEXT("snaphittimes3.txt");
    FString TimeString = FString::Printf(TEXT("%.4f\n"), SnapTime);
    
    if (FFileHelper::SaveStringToFile(TimeString, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Snap time logged to file."));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to log snap time to file."));
    }

    SetRotation(AimRotation);
    FireHitScanWeapon();
    //ServerSetRotation(AimRotation);
}


// Set the rotation of the character 
void AThirdPersonCharacter::SetRotation(FRotator NewRotation)
{
    // Update rotation on the server
    SetActorRotation(NewRotation);
    ReplicatedRotation = NewRotation;

    // Replicate to all clients
    //NetMulticastSetRotation(NewRotation);
}

// Server-side function to set the rotation of the character
void AThirdPersonCharacter::ServerSetRotation_Implementation(FRotator NewRotation)
{
	SetRotation(NewRotation);
}

// Multicast function to set the rotation of the character at all client instances
void AThirdPersonCharacter::NetMulticastSetRotation_Implementation(FRotator NewRotation)
{
    if (!HasAuthority())
    {
        SetActorRotation(NewRotation);
    }
}

// Replicated rotation function
void AThirdPersonCharacter::OnRep_ReplicatedRotation()
{
    SetActorRotation(ReplicatedRotation);

    if (AController* Controller = GetController())
    {
        FRotator ControlRotation = Controller->GetControlRotation();
        ControlRotation.Yaw = ReplicatedRotation.Yaw;
        Controller->SetControlRotation(ControlRotation);
    }
}

// Function to call the running service in the background (if set)
void AThirdPersonCharacter::CallRunningService()
{
    FString Url = TEXT("http://localhost:5000/apply_delay");

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();\

	// Change ariable based on what attack the script is executing
	ArtDelay = true;

    LogCallerClient(1.0f);
    LogCallerServer(1.0f);

	Request->OnProcessRequestComplete().BindUObject(this, &AThirdPersonCharacter::OnServiceResponseReceived);
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->ProcessRequest();
}

void AThirdPersonCharacter::OnServiceResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully applied delay"));
        GetWorld()->GetTimerManager().ClearTimer(TimerHandleLog);
        ResetLogIntervals();
        ArtDelay = false;
    }
    else
    {
        FString ErrorMessage = TEXT("Failed to apply delay");
        if (Response.IsValid())
        {
            ErrorMessage += FString::Printf(TEXT(": %s"), *Response->GetContentAsString());
        }
        else if (!bWasSuccessful)
        {
            ErrorMessage += TEXT(": Request failed");
        }
        UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrorMessage);
    }
}

void AThirdPersonCharacter::ResetLogIntervals()
{
    if (None_Client != -1.0 && None_Server != -1.0)
    {
        UE_LOG(LogTemp, Log, TEXT("Applying 'None' logging preference: ClientInterval=%f, ServerInterval=%f"), None_Client, None_Server);
        LogCallerClient(None_Client);
        LogCallerServer(None_Server);
    }
    else if (Moderate_Client != -1.0 && Moderate_Server != -1.0)
    {
        UE_LOG(LogTemp, Log, TEXT("Applying 'Moderate' logging preference: ClientInterval=%f, ServerInterval=%f"), Moderate_Client, Moderate_Server);
        LogCallerClient(Moderate_Client);
        LogCallerServer(Moderate_Server);
    }
    else if (Extensive_Client != -1.0 && Extensive_Server != -1.0)
    {
        UE_LOG(LogTemp, Log, TEXT("Applying 'Extensive' logging preference: ClientInterval=%f, ServerInterval=%f"), Extensive_Client, Extensive_Server);
        LogCallerClient(Extensive_Client);
        LogCallerServer(Extensive_Server);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No logging preference was set!"));
    }
}



// Set the log interval (based on what player sets as input in the UI)
void AThirdPersonCharacter::SubmitNonePreference()
{
    LogCallerClient(150.0f);
    LogCallerServer(150.0f);
    None_Client = 150.0f;
    None_Server = 150.0f;
}
void AThirdPersonCharacter::SubmitModeratePreference()
{
    LogCallerClient(10.0f);
    LogCallerServer(15.0f);
    Moderate_Client = 10.0f;
    Moderate_Server = 15.0f;
}
void AThirdPersonCharacter::SubmitExtensivePreference()
{
    LogCallerClient(0.1f);
    LogCallerServer(0.1f);
    Extensive_Client = 0.1f;
    Extensive_Server = 0.1f;
}

bool AThirdPersonCharacter::IsCharacterInZone(const FVector& CharacterLocation)
{
    // Extract x and y coordinates of the character
    float X = CharacterLocation.X;

    //UE_LOG(LogTemp, Warning, TEXT("Character Location: X=%.2f Y=%.2f"), X, Y);

    // Check if the character is within the zone boundaries
    return (X >= 3200.00 && X <= 4004.0);
}

void AThirdPersonCharacter::MainLoggerWrapper()
{
    MainLogger();
}

// Main logger function
void AThirdPersonCharacter::MainLogger(bool Shot)
{
    FString FilePath = HasAuthority() ? MainServerPath : MainClientPath;
    FString DirectoryPath = FPaths::GetPath(FilePath);
    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*DirectoryPath))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DirectoryPath);
    }

    // FString FilePath = HasAuthority() ? FPaths::ProjectSavedDir() + TEXT("mainServerPath.json") : FPaths::ProjectSavedDir() + TEXT("mainClientPath.json");

    FString FilePathString = FilePath;
    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("File path is empty!"));
        return;
    }
    
    double Time = HasAuthority() ? (GetWorld()->GetTimeSeconds() - SpawnTime) : GetWorld()->GetTimeSeconds();
    FString PlayerID = GetPlayerIdentifier();
    FVector PlayerLocation = GetActorLocation();
	bool bIsInZone = IsCharacterInZone(PlayerLocation);

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetNumberField(TEXT("Time"), Time);
    JsonObject->SetStringField(TEXT("PlayerID"), PlayerID);
    JsonObject->SetNumberField(TEXT("PlayerLocationX"), PlayerLocation.X);
    JsonObject->SetNumberField(TEXT("PlayerLocationY"), PlayerLocation.Y);
    JsonObject->SetNumberField(TEXT("PlayerLocationZ"), PlayerLocation.Z);
    JsonObject->SetStringField(TEXT("Generated"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
    
    if (bIsInZone)
    {
        if (HasAuthority())
        {
            JsonObject->SetStringField(TEXT("VISIBLE"), TEXT("YES"));
        }
    }
	
    if (Shot)
    {
        double ShotTime = HasAuthority() ? (GetWorld()->GetTimeSeconds() - SpawnTime) : GetWorld()->GetTimeSeconds();
        JsonObject->SetNumberField(TEXT("Shot at Time:"), ShotTime);
        JsonObject->SetNumberField(TEXT("Health"), CurrentHealth);
    }

	if(!HasAuthority() && ArtDelay)
	{
		JsonObject->SetStringField(TEXT("NET.FD"), TEXT("ON"));
	}
	if(!HasAuthority() && LagSwitch)
	{
		JsonObject->SetNumberField(TEXT("Health"), CurrentHealth);
		JsonObject->SetStringField(TEXT("NET.LS"), TEXT("ON"));
	}	
	if(!HasAuthority() && DoS)
	{
		JsonObject->SetNumberField(TEXT("Health"), CurrentHealth);
		JsonObject->SetStringField(TEXT("NET.DOS"), TEXT("ON"));
	}
	else if (HasAuthority() && DoS)
	{
		JsonObject->SetNumberField(TEXT("Health"), CurrentHealth);
	}
	
	if(!HasAuthority() && AimBot)
	{
		JsonObject->SetStringField(TEXT("VIS.AIM"), TEXT("ON"));
	}

    FString JsonString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePathString);

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    if (!FileContent.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
        FJsonSerializer::Deserialize(Reader, JsonArray);
    }

    JsonArray.Add(MakeShareable(new FJsonValueObject(JsonObject)));

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonArray, Writer);

    FFileHelper::SaveStringToFile(OutputString, *FilePathString);
    FString FullDirectoryPath = FPaths::ConvertRelativePathToFull(FilePath);
    UE_LOG(LogTemp, Log, TEXT("Data successfully written to file: %s"), *FullDirectoryPath);

}

// Function to send client data periodically to the server
void AThirdPersonCharacter::SendClientLogData()
{
    if (HasAuthority())
    {
        return;
    }

    FString FilePath = MainClientPath;

    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Client log file path is empty!"));
        return;
    }

    FString FilePathString = FilePath;

    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePathString);

    if (FileContent.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Client log file is empty, nothing to send."));
        return;
    }

    ServerReceiveLogData(FileContent);

    FFileHelper::SaveStringToFile(TEXT(""), *FilePathString);
    UE_LOG(LogTemp, Log, TEXT("Client log file content reset."));
}

// Server-side function to receive client data
void AThirdPersonCharacter::ServerReceiveLogData_Implementation(const FString& LogData)
{
    FString FilePath = MainServerPath;

    if (FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Server log file path is empty!"));
        return;
    }

    FString FilePathString = FilePath;

    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePathString);

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    if (!FileContent.IsEmpty())
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContent);
        FJsonSerializer::Deserialize(Reader, JsonArray);
    }

    TArray<TSharedPtr<FJsonValue>> ClientJsonArray;
    TSharedRef<TJsonReader<>> ClientReader = TJsonReaderFactory<>::Create(LogData);
    FJsonSerializer::Deserialize(ClientReader, ClientJsonArray);

    // Merge client data with server data
    for (auto& ClientJson : ClientJsonArray)
    {
        JsonArray.Add(ClientJson);
    }

    // Sort the array based on the "Time" field
    JsonArray.Sort([](const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B) {
        double TimeA = A->AsObject()->GetNumberField(TEXT("Time"));
        double TimeB = B->AsObject()->GetNumberField(TEXT("Time"));
        return TimeA < TimeB;
    });

    // Write the sorted array back to the file
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonArray, Writer);

    FFileHelper::SaveStringToFile(OutputString, *FilePathString);
    UE_LOG(LogTemp, Log, TEXT("Data successfully written to server log file: %s"), *FilePath);
}

// Get player's identifier 
FString AThirdPersonCharacter::GetPlayerIdentifier() const
{
    APlayerController* Player = GetController<APlayerController>();
    if (Player)
    {
        APlayerState* PlayerState = Player->GetPlayerState<APlayerState>();
        if (PlayerState)
        {
            return FString::FromInt(PlayerState->GetPlayerId());
        }
    }
    return FString(TEXT("Unknown"));
}

// Set the log paths for the server and client
void AThirdPersonCharacter::SetLogPaths()
{
	PlayerNameForServerLog = GetPlayerIdentifier();
	PlayerNameForClientLog = "MyLogs";
}

void AThirdPersonCharacter::LogCallerServer_Implementation(double ServerInterval)
{
    if (HasAuthority())
    {
        // Clear any previous server-side logging timer
        GetWorld()->GetTimerManager().ClearTimer(TimerHandleLog);

        // Set a new timer with the ServerInterval
        UE_LOG(LogTemp, Log, TEXT("Setting server log interval to %f"), ServerInterval);
        GetWorld()->GetTimerManager().SetTimer(TimerHandleLog, this, &AThirdPersonCharacter::MainLoggerWrapper, ServerInterval, true);
    }
}

void AThirdPersonCharacter::LogCallerClient(double ClientInterval)
{
    // Clear any previous server-side logging timer
    GetWorld()->GetTimerManager().ClearTimer(TimerHandleLog);

    // Set a new timer with the ClientInterval
    UE_LOG(LogTemp, Log, TEXT("Setting client log interval to %f"), ClientInterval);
    GetWorld()->GetTimerManager().SetTimer(TimerHandleLog, this, &AThirdPersonCharacter::MainLoggerWrapper, ClientInterval, true);
}