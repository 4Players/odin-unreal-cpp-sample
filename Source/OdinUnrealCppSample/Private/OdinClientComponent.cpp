// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinClientComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "OdinSynthComponent.h"
#include "AudioCaptureBlueprintLibrary.h"
#include "OdinFunctionLibrary.h"
#include "OdinJsonObject.h"
#include "OdinGameInstance.h"

// Sets default values for this component's properties
UOdinClientComponent::UOdinClientComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UOdinClientComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UOdinClientComponent::OnPeerJoinedHandler(int64 PeerId, FString UserId, const TArray<uint8>& UserData,
                                               UOdinRoom* OdinRoom)
{
	// create Json Object from User Data Byte Array
	const auto JSON = UOdinJsonObject::ConstructJsonObjectFromBytes(this, UserData);
	// Get Guid String from Json
	const FString GUIDString = JSON->GetStringField(TEXT("PlayerId"));

	UE_LOG(LogTemp, Warning, TEXT("Peer with PlayerId %s joined. Trying to map to player character ..."), *GUIDString);

	// Parse String into Guid
	FGuid GUID = FGuid();
	if (FGuid::Parse(GUIDString, GUID))
	{
		// if successful, get the UOdinGameInstance and use the PlayerCharacters map to obtain the correct character object
		UOdinGameInstance* GameInstance = Cast<UOdinGameInstance>(UGameplayStatics::GetGameInstance(this));
		if (!GameInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("Received Game Instance of invalid type, please use a UOdinGameInstance."));
			return;
		}

		// Finally add that character together with the Odin Peer Id to the OdinPlayerCharacters map of the Game Instance - for later use in the OnMediaAdded Event
		ACharacter* Character = GameInstance->PlayerCharacters[GUID];
		GameInstance->OdinPlayerCharacters.Add(PeerId, Character);

		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined"), *UserId);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined, but could not be mapped to a player."), *UserId);
	}
}

void UOdinClientComponent::OnMediaAddedHandler(int64 PeerId, UOdinPlaybackMedia* Media, UOdinJsonObject* Properties,
                                               UOdinRoom* OdinRoom)
{
	// get corresponding player character
	UOdinGameInstance* OdinGameInstance = Cast<UOdinGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!OdinGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Received Game Instance of invalid type, please use a UOdinGameInstance."));
		return;
	}
	ACharacter* Player = OdinGameInstance->OdinPlayerCharacters[PeerId];
	// create, attach and cast a new UOdinSynthComponent to the correct player character
	UActorComponent* Comp = Player->AddComponentByClass(UOdinSynthComponent::StaticClass(), false, FTransform::Identity,
	                                                    false);
	UOdinSynthComponent* Synth = Cast<UOdinSynthComponent>(Comp);

	// assign Odin media as usual
	Synth->Odin_AssignSynthToMedia(Media);

	// Here we need to set any wanted attenuation settings
	FSoundAttenuationSettings AttenuationSettings;
	AttenuationSettings.bSpatialize = true;
	AttenuationSettings.bAttenuate = true;
	// more attenuation settings as desired
	Synth->AdjustAttenuation(AttenuationSettings);

	// Lastly activate the Synth Component an we are good to go
	Synth->Activate();
	UE_LOG(LogTemp, Warning, TEXT("Odin Synth Added"));
}

void UOdinClientComponent::OnRoomJoinSuccessHandler(FString RoomId, const TArray<uint8>& RoomUserData, FString Customer,
                                                    int64 OwnPeerId, FString OwnUserId)
{
	UE_LOG(LogTemp, Warning, TEXT("Joined Room"));

	Capture = UOdinFunctionLibrary::CreateOdinAudioCapture(this);

	// cast pointer to capture to UAudioGenerator for Odin_CreateMedia
	UAudioGenerator* CaptureAsGenerator = Cast<UAudioGenerator>(Capture);
	auto Media = UOdinFunctionLibrary::Odin_CreateMedia(CaptureAsGenerator);

	OnAddMediaError.BindDynamic(this, &UOdinClientComponent::OnOdinErrorHandler);

	UOdinRoomAddMedia* Action = UOdinRoomAddMedia::AddMedia(this, Room, Media, OnAddMediaError, OnAddMediaSuccess);
	Action->Activate();

	Capture->StartCapturingAudio();
}

void UOdinClientComponent::OnOdinErrorHandler(int64 ErrorCode)
{
	const FString ErrorString = UOdinFunctionLibrary::FormatError(ErrorCode, true);
	UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorString);
}

void UOdinClientComponent::ConnectToOdin(FGuid PlayerId)
{
	TokenGenerator = UOdinTokenGenerator::ConstructTokenGenerator(this, "AQGEYTtGuFdlq6Msk+bO9ki6dDJ+fG8UmjfZD+VZOuUt");

	RoomToken = TokenGenerator->GenerateRoomToken("Test", "Player", EOdinTokenAudience::Default);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *RoomToken);

	ApmSettings = FOdinApmSettings();

	ApmSettings.bVoiceActivityDetection = true;
	ApmSettings.fVadAttackProbability = 0.7;
	ApmSettings.fVadReleaseProbability = 0.6;
	ApmSettings.bEnableVolumeGate = false;
	ApmSettings.bHighPassFilter = false;
	ApmSettings.GainControllerVersion = EOdinGainControllerVersion::V2;
	ApmSettings.noise_suppression_level = EOdinNoiseSuppressionLevel::OdinNS_Moderate;
	ApmSettings.bTransientSuppresor = false;
	ApmSettings.bEchoCanceller = true;

	Room = UOdinRoom::ConstructRoom(this, ApmSettings);
	Room->onPeerJoined.AddUniqueDynamic(this, &UOdinClientComponent::OnPeerJoinedHandler);
	Room->onMediaAdded.AddUniqueDynamic(this, &UOdinClientComponent::OnMediaAddedHandler);
	OnRoomJoinSuccess.BindDynamic(this, &UOdinClientComponent::OnRoomJoinSuccessHandler);
	OnRoomJoinError.BindDynamic(this, &UOdinClientComponent::OnOdinErrorHandler);

	UE_LOG(LogTemp, Warning, TEXT("Joining Room with PlayerId: %s"), *PlayerId.ToString())

	UOdinJsonObject* JSON = UOdinJsonObject::ConstructJsonObject(this);
	JSON->SetStringField(TEXT("PlayerId"), *PlayerId.ToString());

	const TArray<uint8> UserData = JSON->EncodeJsonBytes();
	UOdinRoomJoin* Action = UOdinRoomJoin::JoinRoom(this, Room, TEXT("https://gateway.odin.4players.io"), RoomToken,
	                                                UserData, FVector(0, 0, 0), OnRoomJoinError, OnRoomJoinSuccess);
	Action->Activate();
}
