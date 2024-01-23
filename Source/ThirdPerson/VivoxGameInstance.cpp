// Fill out your copyright notice in the Description page of Project Settings.

#include "VivoxGameInstance.h"
//#include "VivoxNativeSdk.h"

#define VIVOX_VOICE_SERVER TEXT("https://unity.vivox.com/appconfig/f4554-my_pr-32821-udash")
#define VIVOX_VOICE_DOMAIN TEXT("mtu1xp.vivox.com")
#define VIVOX_VOICE_ISSUER TEXT("f4554-my_pr-32821-udash")
#define VIVOX_VOICE_KEY TEXT("15SBsnqmanVWR0gYJbRnrIItDDMdKyFr")


void UVivoxGameInstance::Init()
{
	Super::Init();
	InitVivox();
}

void UVivoxGameInstance::Shutdown()
{
	Super::Shutdown();
	VivoxVoiceClient->Uninitialize();
}

void UVivoxGameInstance::InitVivox()
{
	VivoxVoiceClient = &static_cast<FVivoxCoreModule*>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")))->VoiceClient();
	VivoxVoiceClient->Initialize();
	Login();
}

FString UVivoxGameInstance::GetFromCmdLine(const FString& arg)
{
	FString ret = TEXT("");
	FParse::Value(FCommandLine::Get(), *arg, ret, true);
		//UE_LOG(LogTemp, Log, TEXT("login: %s"), ret);
	return ret;
}

void UVivoxGameInstance::Login()
{
	FString LoginId = GetFromCmdLine(TEXT("Login="));
	//UE_LOG(LogTemp, Log, TEXT("LoginId: %s"), LoginId);

	LoggedInUserId = AccountId(VIVOX_VOICE_ISSUER, LoginId, VIVOX_VOICE_DOMAIN);
	ILoginSession& MyLoginSession(VivoxVoiceClient->GetLoginSession(LoggedInUserId));

	FTimespan TokenExpiration = FTimespan::FromSeconds(90);
	FString LoginToken = MyLoginSession.GetLoginToken(VIVOX_VOICE_KEY, TokenExpiration);

	ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleted;
	OnBeginLoginCompleted.BindLambda([this, &MyLoginSession](VivoxCoreError)
	{
		UE_LOG(LogTemp, Log, TEXT("Logged into Vivox"));
		//JoinChannel();
		FString nameCurUser;
		nameCurUser = LoggedInUserId.ToString();
		UE_LOG(LogTemp, Log, TEXT("nameCurUser: %s"),*nameCurUser);

		/*if(LoggedInUserId.DisplayName() == TEXT("Hugo"))
		callUser()*/


		
	});

	MyLoginSession.BeginLogin(VIVOX_VOICE_SERVER, LoginToken, OnBeginLoginCompleted);

}

void UVivoxGameInstance::JoinChannel()
{
	ILoginSession& MyLoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
	ChannelId Channel = ChannelId(VIVOX_VOICE_ISSUER, "ChannelId", VIVOX_VOICE_DOMAIN, ChannelType::Echo);
	IChannelSession& ChannelSession = MyLoginSession.GetChannelSession(Channel);
	
	FTimespan TokenExpiration = FTimespan::FromSeconds(90);
	FString JoinToken = ChannelSession.GetConnectToken(VIVOX_VOICE_KEY, TokenExpiration);

	ChannelSession.BeginConnect(true, false, true, JoinToken, NULL);
}

//void UVivoxGameInstance::callUser(const FString& name)
//{
//	ILoginSession& MyLoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
//	FString SessionHandle = TEXT("session:%s", name);
//	FString SessionGroupHandle = TEXT("sessiongroup:%s", name);
//	FString uri = GetFromCmdLine(TEXT("callURI="));
//	FString Passwd = TEXT("0000");
//
//	ILoginSession& MyLoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
//	
//
//	VivoxNativeSdk::Get().InviteUserP2P(LoggedInUserId, SessionHandle);
//}
