// Fill out your copyright notice in the Description page of Project Settings.

#include "UScenario.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY(LogScenario);

UScenario::UScenario()
{
}

UScenario::~UScenario()
{
	Movements.~TArray();
}

void UScenario::InitializeScenarioFromArgs()
{
	// Get Filepath from command line args
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

			int i = 0;
			bool ret = true;
			while (ret)
			{
				const TSharedPtr<FJsonObject>* JsonChildPtr;
				FString index = FString::Printf(TEXT("Move%d"), i);
				ret = JsonObject->TryGetObjectField(index, JsonChildPtr);

				UE_LOG(LogScenario, Log, TEXT("Loop: %d, ret: %d"), i, ret);

				if (ret)
				{
					double timestamp;
					double duration;
					double x;
					double y;
					bool allFieldsFilled = true;
					TSharedPtr<FJsonObject> JsonChild = static_cast<TSharedPtr<FJsonObject>>(*JsonChildPtr);

					if (!JsonChild->TryGetNumberField(TEXT("Timestamp"), timestamp)) break;
					if (!JsonChild->TryGetNumberField(TEXT("Duration"), duration)) break;
					if (!JsonChild->TryGetNumberField(TEXT("X"), x)) break;
					if (!JsonChild->TryGetNumberField(TEXT("Y"), y)) break;

					// fill AutoMove struct representing a schedule movement
					AutoMove Move;
					Move.timestamp = (float)timestamp;
					Move.duration = (float)duration;
					Move.Vector = FVector(x, y, 0);

					UE_LOG(LogScenario, Log, TEXT("Add: {%f %f %f %f}"), Move.timestamp, Move.duration, Move.Vector.X, Move.Vector.Y);

					Movements.Add(Move);

				}
				// EOF: no more move to store
				else
				{
					UE_LOG(LogScenario, Log, TEXT("EOF Json"));
				}
				i++;
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


FVector UScenario::GetMove(double time)
{
	// not efficient
	FVector vect = FVector(0, 0, 0);
	for (AutoMove move : Movements)
	{
		if (time >= move.timestamp && time < (move.timestamp + move.duration))
		{
			// problem if different movements at the same time (only the first one will be returned)
			UE_LOG(LogScenario, Log, TEXT("Time: %f, AutoMove: ts: %f, duration: %f, Vector: X: %f, Y: %f"), 
				time, move.timestamp, move.timestamp + move.duration, move.Vector.X, move.Vector.Y);

			return move.Vector;
		} 
	}
	return vect;
}
