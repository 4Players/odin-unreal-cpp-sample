// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinClientComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "OdinFunctionLibrary.h"
#include "OdinJsonObject.h"
#include "OdinGameInstance.h"
#include "OdinTokenGenerator.h"
#include "OdinAudio/OdinAudioCapture.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinAudio/OdinPipeline.h"
#include "OdinAudio/OdinSynthComponent.h"

UOdinClientComponent::UOdinClientComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UOdinClientComponent::OnPeerJoinedHandler(UOdinRoom* OdinRoom, FOdinPeerJoined PeerData)
{
	if (!IsValid(OdinRoom) || OdinRoom != Room || OdinRoom->IsClosing())
	{
		return;
	}

	UOdinJsonObject* JSON = UOdinJsonObject::ConstructJsonObjectFromBytes(this, PeerData.user_data);

	if (!IsValid(JSON) || !JSON->GetRootObject().IsValid()
		|| !JSON->HasField(TEXT("PlayerId")))
	{
		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined without a PlayerId user data field."), *PeerData.user_id);
		return;
	}
	FString GUIDString = JSON->GetStringField(TEXT("PlayerId"));

	FGuid PlayerId;
	if (!FGuid::Parse(GUIDString, PlayerId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined with an invalid PlayerId."), *PeerData.user_id);
		return;
	}

	UOdinGameInstance* GameInstance = Cast<UOdinGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!IsValid(GameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("Current Game Instance is of invalid type, please use a UOdinGameInstance."));
		return;
	}

	ACharacter* Character = GameInstance->PlayerCharacters.FindRef(PlayerId);
	if (!IsValid(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined, but could not be mapped to a player character."), *PeerData.user_id);
		return;
	}

	CleanupPeerAudio(PeerData.peer_id);

	UOdinDecoder* Decoder = UOdinDecoder::ConstructDecoder(this, 48000, true);

	UActorComponent* Component = Character->AddComponentByClass(UOdinSynthComponent::StaticClass(), false,
		FTransform::Identity, false);
	UOdinSynthComponent* Synth = Cast<UOdinSynthComponent>(Component);

	PeerDecoders.Add(PeerData.peer_id, Decoder);
	PeerSynths.Add(PeerData.peer_id, Synth);
	GameInstance->OdinPlayerCharacters.Add(PeerData.peer_id, Character);

	UOdinFunctionLibrary::RegisterDecoder(Decoder, OdinRoom, PeerData.peer_id);
	Synth->SetDecoder(Decoder);

	FSoundAttenuationSettings AttenuationSettings;
	AttenuationSettings.bSpatialize = true;
	AttenuationSettings.bAttenuate = true;
	// Add further attenuation settings here
	Synth->AdjustAttenuation(AttenuationSettings);
	Synth->Activate();

	UE_LOG(LogTemp, Log, TEXT("Added ODIN playback for peer %s."), *PeerData.user_id);
}

void UOdinClientComponent::OnPeerLeftHandler(UOdinRoom* OdinRoom, FOdinPeerLeft PeerData)
{
	if (OdinRoom == Room)
	{
		// Also called for synthetic peer-left events on connection loss, close and reconnect.
		CleanupPeerAudio(PeerData.peer_id);
	}
}

void UOdinClientComponent::OnRoomJoinSuccessHandler(UOdinRoom* OdinRoom, FOdinJoined Data)
{
	if (!IsValid(OdinRoom) || OdinRoom != Room || OdinRoom->IsClosing())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Joined ODIN room %s."), *Data.room_id);
	CleanupLocalAudio();

	Capture = NewObject<UOdinAudioCapture>(this);
	if (!IsValid(Capture) || !Capture->RestartCapturing(false))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open the microphone. Joined the room without local voice transmission."));
		CleanupLocalAudio();
		return;
	}

	const int32 SampleRate = Capture->GetSampleRate();
	const int32 NumChannels = Capture->GetNumChannels();
	// Peer id 0 lets the plugin resolve the local peer id automatically when sending to the linked room.
	Encoder = UOdinEncoder::ConstructEncoder(this, 0, SampleRate, NumChannels >= 2);
	if (!IsValid(Encoder) || !Encoder->GetHandle())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the ODIN encoder."));
		CleanupLocalAudio();
		return;
	}

	UOdinPipeline* Pipeline = Encoder->GetOrCreatePipeline();
	
	const int32 ApmId = Pipeline->InsertApmEffect(0);
	FOdinApmConfig ApmConfig;
	// For local testing purposes, this is turned off - when running multiple clients on the same machine with the same
	// microphone input, the Echo Canceller will kick in. In production you'd probably want to enable this.
	ApmConfig.echo_canceller = false;
	ApmConfig.noise_suppression = EOdinNoiseSuppression::ODIN_NOISE_SUPPRESSION_MODERATE;
	ApmConfig.high_pass_filter = true;
	ApmConfig.gain_controller = EOdinGainControllerVersion::ODIN_GAIN_CONTROLLER_V2;
	ApmConfig.transient_suppressor = true;

	// The plugin returns 0 on failure.
	if (ApmId <= 0 || !Pipeline->SetApmConfig(ApmId, ApmConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure the ODIN APM effect."));
		CleanupLocalAudio();
		return;
	}

	const int32 VadId = Pipeline->InsertVadEffect(1);
	FOdinVadConfig VadConfig;
	VadConfig.VoiceActivity = {
		.Enabled = true,
		.AttackThreshold = 0.7f,
		.ReleaseThreshold = 0.6f
	};
	VadConfig.VolumeGate = {
		.Enabled = true,
		.AttackThreshold = -30.0f,
		.ReleaseThreshold = -40.0f
	};
	if (VadId <= 0 || !Pipeline->SetVadConfig(VadId, VadConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure the ODIN VAD effect."));
		CleanupLocalAudio();
		return;
	}

	Encoder->SetAudioGenerator(Capture);
	UOdinFunctionLibrary::LinkEncoderToRoom(Encoder, OdinRoom);

	Capture->StartCapturingAudio();
}

void UOdinClientComponent::OnRoomErrorHandler(UOdinRoom* OdinRoom, FOdinError Data)
{
	if (OdinRoom == Room)
	{
		UE_LOG(LogTemp, Error, TEXT("ODIN room error: %s"), *Data.message);
	}
}

void UOdinClientComponent::CleanupLocalAudio()
{
	if (IsValid(Capture))
	{
		Capture->StopCapturingAudio();
	}
	if (IsValid(Encoder))
	{
		// FreeEncoder invalidates the handle used by the capture callback; GC removes its delegate.
		UOdinFunctionLibrary::UnlinkEncoderFromRoom(Encoder);
		UOdinEncoder::FreeEncoder(Encoder);
	}
	Encoder = nullptr;
	Capture = nullptr;
}

void UOdinClientComponent::CleanupPeerAudio(int64 PeerId)
{
	const bool bIsPlayerTracked = PeerDecoders.Contains(PeerId) || PeerSynths.Contains(PeerId);
	UOdinSynthComponent* Synth = nullptr;
	if (PeerSynths.RemoveAndCopyValue(PeerId, Synth) && IsValid(Synth))
	{
		Synth->Stop();
		Synth->DestroyComponent();
	}

	UOdinDecoder* Decoder = nullptr;
	if (PeerDecoders.RemoveAndCopyValue(PeerId, Decoder) && IsValid(Decoder))
	{
		UOdinDecoder::FreeDecoder(Decoder);
	}

	if (bIsPlayerTracked)
	{
		UWorld* World = GetWorld();
		UOdinGameInstance* GameInstance = World ? Cast<UOdinGameInstance>(World->GetGameInstance()) : nullptr;
		if (IsValid(GameInstance))
		{
			GameInstance->OdinPlayerCharacters.Remove(PeerId);
		}
	}
}

void UOdinClientComponent::ConnectToOdin(FGuid PlayerId)
{
	if (AccessKey.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ODIN Access Key is empty. Set it on the OdinClient component in the editor or Blueprint before connecting."));
		return;
	}

	UOdinTokenGenerator* TokenGenerator = UOdinTokenGenerator::ConstructTokenGenerator(this, AccessKey);
	if (!IsValid(TokenGenerator) || !TokenGenerator->GetHandle())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the ODIN token generator. Please check the Access Key."));
		return;
	}

	UOdinJsonObject* AuthJson = nullptr;
	FString RoomToken;
	TokenGenerator->GenerateRoomToken(TEXT("TestRoom"), TEXT("Player"), AuthJson, RoomToken);
	TokenGenerator->ReleaseHandle();
	if (!IsValid(AuthJson) || RoomToken.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Generate room token failed. Please check the Access Key."));
		return;
	}

	DisconnectFromOdin();
	if (IsValid(Room))
	{
		// Use free room to instantly shut down the old room.
		Room->FreeRoom();
	}
	Room = UOdinRoom::ConstructRoom(this);
	if (!IsValid(Room))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the ODIN room."));
		return;
	}

	Room->OnRoomPeerJoinedBP.AddUniqueDynamic(this, &UOdinClientComponent::OnPeerJoinedHandler);
	Room->OnRoomPeerLeftBP.AddUniqueDynamic(this, &UOdinClientComponent::OnPeerLeftHandler);
	Room->OnRoomJoinedBP.AddUniqueDynamic(this, &UOdinClientComponent::OnRoomJoinSuccessHandler);
	Room->OnRoomErrorBP.AddUniqueDynamic(this, &UOdinClientComponent::OnRoomErrorHandler);

	UOdinJsonObject* UserDataObject = UOdinJsonObject::ConstructJsonObject(this);
	UserDataObject->SetStringField(TEXT("PlayerId"), PlayerId.ToString());
	AuthJson->SetStringField(TEXT("user_data"), UserDataObject->EncodeJson());

	bool bSuccess = false;
	Room->ConnectRoom(TEXT("https://gateway.odin.4players.io"), AuthJson->EncodeJson(), bSuccess);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initiate the ODIN connection."));
		DisconnectFromOdin();
		Room = nullptr;
	}
}

void UOdinClientComponent::DisconnectFromOdin()
{
	CleanupLocalAudio();

	// cleanup all peer audio playback
	TArray<int64> PeerIds;
	PeerDecoders.GetKeys(PeerIds);
	for (int64 PeerId : PeerIds)
	{
		CleanupPeerAudio(PeerId);
	}
	
	if (IsValid(Room))
	{
		Room->OnRoomPeerJoinedBP.RemoveDynamic(this, &UOdinClientComponent::OnPeerJoinedHandler);
		Room->OnRoomPeerLeftBP.RemoveDynamic(this, &UOdinClientComponent::OnPeerLeftHandler);
		Room->OnRoomJoinedBP.RemoveDynamic(this, &UOdinClientComponent::OnRoomJoinSuccessHandler);
		Room->OnRoomErrorBP.RemoveDynamic(this, &UOdinClientComponent::OnRoomErrorHandler);
		Room->CloseRoom();
	}
}

void UOdinClientComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DisconnectFromOdin();
	if (IsValid(Room))
	{
		Room->FreeRoom();
	}
	Room = nullptr;
	Super::EndPlay(EndPlayReason);
}
