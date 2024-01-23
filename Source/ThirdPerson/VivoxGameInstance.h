#pragma once

#include "VivoxCore.h"
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VivoxGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSON_API UVivoxGameInstance : public UGameInstance
{
	GENERATED_BODY()
	virtual void Init() override;
	virtual void Shutdown() override;

	void InitVivox();
	void Login();
	void JoinChannel();
	FString GetFromCmdLine(const FString& name);
	void callUser(const FString& name);

	IClient* VivoxVoiceClient;
	AccountId LoggedInUserId;
};
