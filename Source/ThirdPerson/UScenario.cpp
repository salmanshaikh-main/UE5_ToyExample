#include "UScenario.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogScenario);

UScenario::UScenario()
{
}

UScenario::~UScenario()
{
    Movements.~TArray();
    Shootings.~TArray();
}

void UScenario::InitializeScenarioFromArgs()
{
    FString Path;
    if (FParse::Value(FCommandLine::Get(), TEXT("Path="), Path, true))
    {
        DeserializeJsonFile(Path);
    }
    else
    {
        UE_LOG(LogScenario, Warning, TEXT("No path specified"));
    }
}

void UScenario::DeserializeJsonFile(const FString& FilePath)
{
    FString JsonString;
    if (FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
        if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
        {
            UE_LOG(LogScenario, Log, TEXT("Successfully parsed the Json file!"));
            
            // Get all keys and sort them
            TArray<FString> Keys;
            JsonObject->Values.GetKeys(Keys);
            Keys.Sort([](const FString& A, const FString& B) {
                FString TypeA = A.Left(4);
                FString TypeB = B.Left(4);
                int32 IndexA = FCString::Atoi(*A.Right(A.Len() - 4));
                int32 IndexB = FCString::Atoi(*B.Right(B.Len() - 4));
                if (TypeA == TypeB) return IndexA < IndexB;
                return TypeA < TypeB;
            });

            for (const FString& Key : Keys)
            {
                const TSharedPtr<FJsonObject>* JsonChildPtr;
                if (JsonObject->TryGetObjectField(Key, JsonChildPtr))
                {
                    TSharedPtr<FJsonObject> JsonChild = *JsonChildPtr;
                    if (Key.StartsWith(TEXT("Move")))
                    {
                        double timestamp, duration, x, y;
                        if (JsonChild->TryGetNumberField(TEXT("Timestamp"), timestamp) &&
                            JsonChild->TryGetNumberField(TEXT("Duration"), duration) &&
                            JsonChild->TryGetNumberField(TEXT("X"), x) &&
                            JsonChild->TryGetNumberField(TEXT("Y"), y))
                        {
                            AutoMove Move;
                            Move.timestamp = (float)timestamp;
                            Move.duration = (float)duration;
                            Move.Vector = FVector(x, y, 0);
                            UE_LOG(LogScenario, Log, TEXT("Add Move: {%f %f %f %f}"), Move.timestamp, Move.duration, Move.Vector.X, Move.Vector.Y);
                            Movements.Add(Move);
                        }
                    }
                    else if (Key.StartsWith(TEXT("Shoot")))
                    {
                        double timestamp;
                        FString button;
                        if (JsonChild->TryGetNumberField(TEXT("Timestamp"), timestamp) &&
                            JsonChild->TryGetStringField(TEXT("Button"), button))
                        {
                            AutoShoot Shoot;
                            Shoot.timestamp = (float)timestamp;
                            Shoot.button = button;
                            UE_LOG(LogScenario, Log, TEXT("Add Shoot: {%f %s}"), Shoot.timestamp, *Shoot.button);
                            Shootings.Add(Shoot);
                        }
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogScenario, Log, TEXT("Failed to deserialize JSON"));
        }
    }
    else
    {
        UE_LOG(LogScenario, Log, TEXT("Failed to load JSON file: %s"), *FilePath);
    }
}

FVector UScenario::GetMove(double time, FVector CurrentLocation)
{
    FVector vect = FVector(0, 0, 0);
    for (AutoMove move : Movements)
    {
        if (time >= move.timestamp && time < (move.timestamp + move.duration))
        {
            UE_LOG(LogScenario, Log, TEXT("Time: %f, AutoMove: ts: %f, duration: %f, Vector: X: %f, CurrentLocation X: %f, Y: %f, CurrentLocation Y: %f"), 
                time, move.timestamp, move.timestamp + move.duration, move.Vector.X, CurrentLocation.X, move.Vector.Y, CurrentLocation.Y);

            return move.Vector;
        } 
    }
    return vect;
}

bool UScenario::HandleShooting(double time, APlayerController* PlayerController)
{
    for (int i = 0; i < Shootings.Num(); ++i)
    {
        AutoShoot& shoot = Shootings[i];
        if (time >= shoot.timestamp)
        {
            UE_LOG(LogScenario, Log, TEXT("Time: %f, AutoShoot: ts: %f, Button: %s"), time, shoot.timestamp, *shoot.button);
            
            if (shoot.button.Equals("Left", ESearchCase::IgnoreCase))
            {
                // Simulate left mouse click
                SimulateMouseClick(EKeys::LeftMouseButton, PlayerController);
            }
            else if (shoot.button.Equals("Right", ESearchCase::IgnoreCase))
            {
                // Simulate right mouse click
                SimulateMouseClick(EKeys::RightMouseButton, PlayerController);
            }
            
            // Remove the executed shoot from the array
            Shootings.RemoveAt(i);
            return true;
        }
    }
    // If no shooting was executed, return false
    return false;
}

void UScenario::SimulateMouseClick(FKey Key, APlayerController* PlayerController)
{
    if (PlayerController)
    {
        // Simulate mouse button press
        PlayerController->InputKey(Key, EInputEvent::IE_Pressed, 1.0f, false);

        // Simulate mouse button release
        PlayerController->InputKey(Key, EInputEvent::IE_Released, 1.0f, false);
    }
}
