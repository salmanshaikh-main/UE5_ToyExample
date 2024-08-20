#include "ThirdPersonCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "ThirdPersonProjectile.h"
#include "APICallerActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include <iostream>
#include <fstream> 
#include <ctime>
#include <chrono>
#include "Misc/DateTime.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "HAL/PlatformProcess.h"
#include "Blueprint/UserWidget.h"  // For UUserWidget
#include "Kismet/GameplayStatics.h"  // For UGameplayStatics
#include "Sockets.h"
#include "IPAddress.h"
#include "ThirdPersonGameMode.h"
#include "Networking.h"





DEFINE_LOG_CATEGORY(LogTemplateCharacter);

FString FullServerFilePath;
FString FullClientFilePath;
FString ServerPath;
FString ClientPath;

//////////////////////////////////////////////////////////////////////////
// AThirdPersonCharacter

AThirdPersonCharacter::AThirdPersonCharacter()
{
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

	APIManager = CreateDefaultSubobject<UAPIManager>(TEXT("APIManager"));
}

void AThirdPersonCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	//OnHealthUpdate();
	
	// Init the Scenario
	Scenario.InitializeScenarioFromArgs();

	FString CurrentTime = FDateTime::Now().ToString(TEXT("%H:%M:%S"));

	if (HasAuthority()) {
		if (FParse::Value(FCommandLine::Get(), TEXT("ServerLog="), ServerPath, true)) {
			FullServerFilePath = FPaths::Combine(TEXT("../../Logs/"), ServerPath + TEXT("_") + CurrentTime + TEXT(".txt"));
		}
	}
	else {
		if (FParse::Value(FCommandLine::Get(), TEXT("ClientLog="), ClientPath, true)) {
			FullClientFilePath = FPaths::Combine(TEXT("../../Logs/"), ClientPath + TEXT("_") + CurrentTime + TEXT(".txt"));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

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

		PlayerInputComponent->BindAction("DoS", IE_Pressed, this, &AThirdPersonCharacter::ClientInvokeRPC);

		//widget
		//PlayerInputComponent->BindAction("BringWidgetUp", IE_Pressed, this, &AThirdPersonCharacter::ShowTextInputWidget);
	//--------------------------------------------------------------------------------------------------------------------------------
		//PlayerInputComponent->BindAction("FromClient", IE_Pressed, this, &AThirdPersonCharacter::ExecuteScript);

		PlayerInputComponent->BindAction("CallService", IE_Pressed, this, &AThirdPersonCharacter::CallRunningService);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


// bool AThirdPersonCharacter::IsPythonInstalled()
// {
//     FString Command = TEXT("python --version");
//     #if !PLATFORM_WINDOWS
//         Command = TEXT("python3 --version");
//     #endif
//     int32 ReturnCode = 0;
//     FString StdOut;
//     FString StdErr;
//     FPlatformProcess::ExecProcess(*Command, nullptr, &ReturnCode, &StdOut, &StdErr);
//     return ReturnCode == 0;
// }

// void AThirdPersonCharacter::ExecuteScript()
// {
//     FString ScriptPath;

// #if PLATFORM_WINDOWS
//     ScriptPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/example_script.bat"));
// #elif PLATFORM_LINUX
//     ScriptPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/example_script.sh"));
//     // ScriptPath = FPaths::ConvertRelativePathToFull(ScriptPath); // Convert to absolute path
//     // FString Command = TEXT("python3");
//     // FString QuotedScriptPath = FString::Printf(TEXT("\"%s\""), *ScriptPath); // Enclose path in quotes
//     // TArray<FString> Arguments;
//     // Arguments.Add(QuotedScriptPath);
// #elif PLATFORM_MAC
//     ScriptPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/example_script.sh"));
// #else
//     UE_LOG(LogTemp, Warning, TEXT("Unsupported platform!"));
//     return;
// #endif

// 	if (FPaths::FileExists(ScriptPath))
// 	{
// 		FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*Command, *Arguments[0], true, false, false, nullptr, 0, nullptr, nullptr);
		
// 		if (!ProcessHandle.IsValid())
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("Failed to start process: %s"), *ScriptPath);
// 		}
// 		else
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("Script executed successfully: %s"), *ScriptPath);
// 		}
// 	}
// 	else
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Script file does not exist: %s"), *ScriptPath);
// 	}
// }


void AThirdPersonCharacter::ClientInvokeRPC()
{
	GetIds();
}

// Server-side function implementation
void AThirdPersonCharacter::GetIds_Implementation()
{
    if (HasAuthority()) // Ensure this runs on the server
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        TArray<TSharedPtr<FJsonValue>> PlayersArray;
        // Get the player controller of the invoking client
        APlayerController* InvokingPlayerController = Cast<APlayerController>(GetController());
        bool bOtherPlayersFound = false;

        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC != InvokingPlayerController) // Exclude the invoking player controller
            {
                UNetConnection* NetConnection = Cast<UNetConnection>(PC->Player);
                if (NetConnection)
                {
                    bOtherPlayersFound = true;
                    TSharedPtr<FJsonObject> PlayerInfo = MakeShareable(new FJsonObject());
                    FString IPAddressString = NetConnection->GetRemoteAddr()->ToString(false);
                    PlayerInfo->SetStringField("IPAddress", IPAddressString);
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
                MessageToDisplay += FString::Printf(TEXT("Player IP Address: %s"), *IPAddress);
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
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, MessageToDisplay);
        }
    }
}

void AThirdPersonCharacter::SubmitButton(const FString& PlayerInput)
{
    if (HasAuthority()) // Checks if this is the server
    {
        ServerHandlePlayerInput(PlayerInput);
    }
    else
    {
        // Clients should call this function to send the data to the server
        ServerHandlePlayerInput(PlayerInput);
    }
}

void AThirdPersonCharacter::ServerHandlePlayerInput_Implementation(const FString& PlayerInput)
{
    // Perform IP address validation
    FIPv4Address IPAddress;
    if (!FIPv4Address::Parse(PlayerInput, IPAddress))
    {
        ClientNotifyValidInput(FString("Invalid IP address format!"));
        return;
    }

    // Iterate through all connected players except the one invoking this method
	APlayerController* TargetedPlayerController = nullptr;
	APlayerController* InvokingPlayerController = Cast<APlayerController>(GetController());
	FString InvokingPlayerName = InvokingPlayerController ? InvokingPlayerController->PlayerState->GetPlayerName() : FString();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC != InvokingPlayerController)  // Exclude the invoking player controller
		{
			UNetConnection* NetConnection = Cast<UNetConnection>(PC->Player);
			if (NetConnection)
			{
				FString ConnectedIPAddress = NetConnection->GetRemoteAddr()->ToString(false);

				// Compare the input IP address with the connected IP address
				if (ConnectedIPAddress.Equals(PlayerInput))
				{
					FString PlayerName = PC->PlayerState->GetPlayerName();
					if (!PlayerName.Equals(InvokingPlayerName))  // Exclude player by name
					{
						TargetedPlayerController = PC;  // Store the targeted player's PlayerController
						break;
					}
				}
			}
		}
	}


	if (TargetedPlayerController)
	{        
		ClientNotifyValidInput(FString("Player with IP address found! Applied DoS attack!"));
		AThirdPersonCharacter* TargetedCharacter = Cast<AThirdPersonCharacter>(TargetedPlayerController->GetPawn());
		if (TargetedCharacter)
		{
			AThirdPersonGameMode* GameMode = Cast<AThirdPersonGameMode>(GetWorld()->GetAuthGameMode());
			if (GameMode)
			{
				GameMode->SimulateDoSAttack(TargetedPlayerController, 25, 95, 15.0f);
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

void AThirdPersonCharacter::Client_SetPacketLoss_Implementation(int OutLossRate, int InLossRate)
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        // Command to set outgoing packet loss
        FString Command = FString::Printf(TEXT("NetEmulation.PktLoss %d"), OutLossRate);
        PC->ConsoleCommand(*Command);

        // Command to set incoming packet loss
        FString Command2 = FString::Printf(TEXT("NetEmulation.PktIncomingLoss %d"), InLossRate);
        PC->ConsoleCommand(*Command2);

        // Display success message on the screen
        // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Packet loss rates set to %d%% (outgoing) and %d%% (incoming)"), OutLossRate, InLossRate));
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to find PlayerController for setting packet loss"));
    }
}



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
    // You can also trigger a UI update to show this message to the player
    // For example, using UMG to display the message
}

void AThirdPersonCharacter::TargetedClientReceiveMessage_Implementation(const FString& Message)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
}

void AThirdPersonCharacter::CallRunningService()
{
    FString Url = TEXT("http://localhost:5000/apply_delay");

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AThirdPersonCharacter::OnServiceResponseReceived);
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->ProcessRequest();
}

void AThirdPersonCharacter::OnServiceResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response->GetResponseCode() == 200)
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully applied delay"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to apply delay: %s"), *Response->GetContentAsString());
    }
}

void AThirdPersonCharacter::ActivateSuperpower()
{
    if (APIManager)
    {
        APIManager->ActivateSuperpower();
    }
}


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

		FVector PlayerLocation = GetActorLocation();

		//UE_LOG(LogTemplateCharacter, Log, TEXT("Applying Movement: %f, %f, PLayer Location: %f %f"), MovementVector.X, MovementVector.Y, PlayerLocation.X, PlayerLocation.Y);
	}
}

// Replicated Properties

void AThirdPersonCharacter::GetLifetimeReplicatedProps(TArray <FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(AThirdPersonCharacter, CurrentHealth);
}

void AThirdPersonCharacter::OnHealthUpdate()
{
	//Client-specific functionality
	if (IsLocallyControlled())
	{
		// FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			//FString deathMessage = FString::Printf(TEXT("You have been killed."));
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
			FVector RespawnLocation = FVector(1950.00, 30, 322.15);
			ServerRespawn(RespawnLocation);
			//FString respawnMessage = FString::Printf(TEXT("You have respawned with full health"));
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, respawnMessage);
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
// void AThirdPersonCharacter::ServerPlayerDied_Implementation()
// {
//     // Handle server-specific logic here
//     // Restart the level or perform any other server-specific actions
//     // For example, you can call a function to restart the level on the server

//     // RestartLevelOnServer();
// }

// Function to restart the level on the server
// void AThirdPersonCharacter::RestartLevelOnServer()
// {
//     // Check if running on the server
//     if (HasAuthority())
//     {
//         // Get the current world
//         UWorld* World = GetWorld();

//         if (World)
//         {
//             // Specify the map name to travel to
//             FString MapName = "ThirdPersonMap";

//             // Use ServerTravel to restart the level on the server
//             World->ServerTravel(MapName);
//         }
//     }
// }


void AThirdPersonCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}

void AThirdPersonCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

float AThirdPersonCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}

void AThirdPersonCharacter::MoveToPosition(FVector TargetLocation)
{
	// FVector CurrentLocation = GetActorLocation();
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction);
	//FollowCamera->SetWorldRotation(FRotator(0, 90, 0));
	//CameraBoom->SetWorldLocation(GetActorLocation()); // not realy sure for the camera, look into cameraboom if we want somthing specific for the cam
	UE_LOG(LogTemplateCharacter, Log, TEXT("Auto Move to position: %f %f %f"), Direction.X, Direction.Y, Direction.Z);
	//FRotator NewControlRotation = Direction.Rotation(); // Convert direction to rotation
	//NewControlRotation.Pitch = 0; // Keep the pitch level (no up/down tilt)
	//NewControlRotation.Roll = 0; // Keep the roll level (no tilt)
	//if (AController* Controller = GetController())
	//{
	//	Controller->SetControlRotation(NewControlRotation); // Set the controller rotation to face the direction
	//}
}


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


void AThirdPersonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLocation = GetActorLocation();

	uint64 CurrentFrameNumber = GFrameCounter;

	UWorld* World = GetWorld();
	FVector PlayerLocation = GetActorLocation();

	double timeSeconds;

	FString LocationString = FString::Printf(TEXT("Current Location: X=%.2f Y=%.2f Z=%.2f"), CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z);
    GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow, LocationString);
	

	if (World)
	{
		timeSeconds = World->GetTimeSeconds();

		auto currentTime = std::chrono::system_clock::now();
		auto durationSinceEpoch = currentTime.time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch);
		auto nanoseconds = durationSinceEpoch - seconds;

		long long int totalSeconds = seconds.count();
		long long int totalNanoseconds = nanoseconds.count();


		UE_LOG(LogTemplateCharacter, Verbose, TEXT("time: %lld"), currentTime);

		//if (GetLocalRole() == ROLE_Authority)
		// {
		// 	LogMovement();
		// }

		if (HasAuthority())
		{
			WriteMovementDataToJson(FullServerFilePath, PlayerLocation, timeSeconds, CurrentFrameNumber, totalSeconds, totalNanoseconds);
		}
		else {
			WriteMovementDataToJson(FullClientFilePath, PlayerLocation, timeSeconds, CurrentFrameNumber, totalSeconds, totalNanoseconds);
		}
	}
	else return;
	
	// auto moves
	FVector Vector;
	Vector = Scenario.GetMove(timeSeconds, CurrentLocation);

	// not move when vector at 0		
	// if vector is zero, send one last move to the last position
	if (!Vector.IsZero())
	{
		MoveToPosition(Vector);
	}
	//bool shootDone = false;
	//if (AController* Controller = GetController())
	//{
	//	shootDone = Scenario.HandleShooting(timeSeconds, Cast<APlayerController>(Controller));
	//}
}

void AThirdPersonCharacter::LogMovement()
{
    FVector CurrentLocation = GetActorLocation();
    UE_LOG(LogTemplateCharacter, Log, TEXT("Player %s moved to %s"),
           *GetPlayerIdentifier(), *CurrentLocation.ToString());
}

FString AThirdPersonCharacter::GetPlayerIdentifier() const
{
    APlayerController* Player = GetController<APlayerController>();
    if (Player)
    {
        return Player->GetName();
    }
    return FString(TEXT("Unknown"));
}

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

void AThirdPersonCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AThirdPersonCharacter::HandleFire_Implementation(const FVector& spawnLocation, const FRotator& spawnRotation)
{
	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AThirdPersonProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonProjectile>(spawnLocation, spawnRotation, spawnParameters);

}

void AThirdPersonCharacter::APIRequest()
{
	FString PlayerID;
    // Assuming you have a way to get the PlayerID, e.g., from the PlayerState
	APlayerState* PS = GetPlayerState();
	PlayerID = PS->GetPlayerName();

    ServerCallAPI(PlayerID);
}


void AThirdPersonCharacter::ServerCallAPI_Implementation(const FString& PlayerID)
{
    // This function will only be executed on the server
    UE_LOG(LogTemp, Log, TEXT("ServerCallAPI_Implementation for Player called on server side %s"), *PlayerID);
	//FVector PlayerLocation = GetActorLocation();
    
    // Spawn the APICaller actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    AAPICallerActor* APICaller = GetWorld()->SpawnActor<AAPICallerActor>(AAPICallerActor::StaticClass(), GetActorLocation(), GetActorRotation(), SpawnParams);
    if (APICaller)
    {
        // Attach the APICaller actor to the player character
        APICaller->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

        // Call the API
        APICaller->CallAPI(PlayerID);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn APICaller actor"));
    }
}


void AThirdPersonCharacter::WriteMovementDataToJson(const FString& FilePath, const FVector& PlayerLocation, double TimeSeconds, uint64 FrameNumber, long long int TimeSec, long long int TimeNano)
{
	// Create a JSON object to store movement data
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	// Add movement data to the JSON object
	JsonObject->SetNumberField(TEXT("PlayerLocationX"), PlayerLocation.X);
	JsonObject->SetNumberField(TEXT("PlayerLocationY"), PlayerLocation.Y);
	JsonObject->SetNumberField(TEXT("Time"), TimeSeconds);
	JsonObject->SetNumberField(TEXT("TimeSec"), TimeSec);
	JsonObject->SetNumberField(TEXT("TimeNano"), TimeNano);
	JsonObject->SetNumberField(TEXT("FrameNumber"), FrameNumber);
	// Convert JSON object to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	//UE_LOG(LogTemplateCharacter, Log, TEXT("JsonString: %s"), *JsonString);

	// Write the JSON string to the file
	std::ofstream MyFile(TCHAR_TO_UTF8(*FilePath), std::ios::app);
	MyFile << TCHAR_TO_UTF8(*JsonString);
	MyFile.close();
}

void AThirdPersonCharacter::FireHitScanWeapon()
{
    FVector SpawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 200.0f) + (GetActorUpVector() * 62.0f);
    FRotator SpawnRotation = GetActorRotation();
    SpawnRotation.Yaw -= 2.0f;

    FVector Start = SpawnLocation;
    FVector End = Start + (SpawnRotation.Vector() * HitScanRange);

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
		//if (HitActor && HitActor->CanBeDamaged())
		//{
			// Perform additional server-side validation here if needed
			// (e.g., check distance, line of sight, etc.)

			// Apply damage
			//UGameplayStatics::ApplyPointDamage(HitActor, HitScanDamage, (HitLocation - GetActorLocation()).GetSafeNormal(), FHitResult(), GetInstigatorController(), this, UDamageType::StaticClass());
		//}
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

bool AThirdPersonCharacter::ServerValidateHitScanDamage_Validate(AActor* HitActor, FVector_NetQuantize HitLocation)
{
    return true;
}

void AThirdPersonCharacter::ServerValidateHitScanDamage_Implementation(AActor* HitActor, FVector_NetQuantize HitLocation)
{
    // This function runs on the server
    if (HitActor && HitActor->CanBeDamaged())
    {
        // Perform additional server-side validation here if needed
        // (e.g., check distance, line of sight, etc.)

        // Apply damage
        UGameplayStatics::ApplyPointDamage(HitActor, HitScanDamage, (HitLocation - GetActorLocation()).GetSafeNormal(), FHitResult(), GetInstigatorController(), this, UDamageType::StaticClass());
    }
}