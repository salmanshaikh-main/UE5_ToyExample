// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include <iostream>
#include <fstream>  



DEFINE_LOG_CATEGORY(LogTemplateCharacter);

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

	// Init the Scenario
	Scenario.InitializeScenarioFromArgs();

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
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
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

		// Write movement data to JSON file
        WriteMovementDataToJson(MovementVector, PlayerLocation);
		//UE_LOG(LogTemplateCharacter, Log, TEXT("Applying Movement: %f, %f, PLayer Location: %f %f"), MovementVector.X, MovementVector.Y, PlayerLocation.X, PlayerLocation.Y);
	}
}


void AThirdPersonCharacter::MoveToPosition(FVector TargetLocation)
{
	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Normalize();
	AddMovementInput(Direction);
	CameraBoom->SetWorldLocation(GetActorLocation()); // not realy sure for the camera, look into cameraboom if we want somthing specific for the cam
	UE_LOG(LogTemplateCharacter, Log, TEXT("Auto Move to position: %f %f %f"), Direction.X, Direction.Y, Direction.Z);
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
	
	UWorld* World = GetWorld();
	double timeSeconds;
	if (World)
	{
		timeSeconds = World->GetTimeSeconds();
	}
	else return;

	// auto moves
	FVector Vector;
	Vector = Scenario.GetMove(timeSeconds);

	// not move when vector at 0		
	if (!Vector.IsZero())
	{		
		MoveToPosition(Vector);
	}

	
}

void AThirdPersonCharacter::WriteMovementDataToJson(const FVector2D& MovementVector, const FVector& PlayerLocation)
{
    // Create a JSON object to store movement data
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    // Add movement data to the JSON object
    JsonObject->SetNumberField(TEXT("MovementVectorX"), MovementVector.X);
    JsonObject->SetNumberField(TEXT("MovementVectorY"), MovementVector.Y);
    JsonObject->SetNumberField(TEXT("PlayerLocationX"), PlayerLocation.X);
    JsonObject->SetNumberField(TEXT("PlayerLocationY"), PlayerLocation.Y);

    // Convert JSON object to string
    FString JsonString;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	UE_LOG(LogTemplateCharacter, Log, TEXT("JSON String: %s"), *JsonString);

	// Write the JSON string to a file
	FString FilePath = FPaths::ProjectDir() + TEXT("MovementData.json");

	std::ofstream MyFile("logs.txt", std::ios::app);
  	MyFile << "test";
	MyFile.close();

	// Check if the file exists
	if (FPaths::FileExists(FilePath))
	{
		// Load existing content
		FString ExistingContent;
		FFileHelper::LoadFileToString(ExistingContent, *FilePath);

		// Append the new JSON string
		ExistingContent += JsonString;

		// Save the updated content back to the file
		bool bSaved = FFileHelper::SaveStringToFile(ExistingContent, *FilePath);

		if (bSaved)
		{
			// File saved successfully
			UE_LOG(LogTemp, Warning, TEXT("JSON file updated and saved successfully at: %s"), *FilePath);
		}
		else
		{
			// Failed to save the updated file
			UE_LOG(LogTemp, Error, TEXT("Failed to update JSON file at: %s"), *FilePath);
		}
	}
	else
	{
		// File doesn't exist, create a new file and add content
		bool bSaved = FFileHelper::SaveStringToFile(JsonString, *FilePath);

		if (bSaved)
		{
			// File created and saved successfully
			UE_LOG(LogTemp, Warning, TEXT("JSON file created and saved successfully at: %s"), *FilePath);
		}
		else
		{
			// Failed to create and save the file
			UE_LOG(LogTemp, Error, TEXT("Failed to create JSON file at: %s"), *FilePath);
		}
	}

}