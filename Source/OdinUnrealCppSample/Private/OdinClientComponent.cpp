// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinClientComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "AudioCaptureBlueprintLibrary.h"
#include "OdinFunctionLibrary.h"
#include "OdinJsonObject.h"
#include "OdinGameInstance.h"
#include "OdinAudio/OdinDecoder.h"
#include "OdinAudio/OdinEncoder.h"
#include "OdinAudio/OdinPipeline.h"
#include "OdinAudio/OdinSynthComponent.h"

// Sets default values for this component's properties
UOdinClientComponent::UOdinClientComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UOdinClientComponent::OnPeerJoinedHandler(UOdinRoom* OdinRoom, FOdinPeerJoined PeerData)
{
	// create Json Object from User Data Byte Array
	const auto JSON = UOdinJsonObject::ConstructJsonObjectFromBytes(this, PeerData.user_data);
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

		ACharacter* Character = GameInstance->PlayerCharacters.FindRef(GUID);

		if (Character)
		{
			GameInstance->OdinPlayerCharacters.Add(PeerData.peer_id, Character);

			// Create a Decoder with 48000hz sample rate and choose the Channel mode (true for stereo, false for mono)
			UOdinDecoder* Decoder = UOdinDecoder::ConstructDecoder(this, 48000, true);

			// Register the decoder to the room and peer
			UOdinFunctionLibrary::RegisterDecoder(Decoder, OdinRoom, PeerData.peer_id);

			// Create Odin Synth Component on the Character
			UActorComponent* Comp = Character->AddComponentByClass(UOdinSynthComponent::StaticClass(), false,
			                                                       FTransform::Identity, false);
			UOdinSynthComponent* Synth = Cast<UOdinSynthComponent>(Comp);
			// Assign Decoder to Synth
			Synth->SetDecoder(Decoder);

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

		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined"), *PeerData.user_id);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Peer %s joined, but could not be mapped to a player."), *PeerData.user_id);
	}
}


void UOdinClientComponent::OnRoomJoinSuccessHandler(UOdinRoom* OdinRoom, FOdinJoined Data)
{
	UE_LOG(LogTemp, Warning, TEXT("Joined Room"));

	Capture = UOdinFunctionLibrary::CreateOdinAudioCapture(this);

	// cast pointer to capture to UAudioGenerator for CreateOdinEncoderFromGenerator
	UAudioGenerator* CaptureAsGenerator = Cast<UAudioGenerator>(Capture);
	Encoder = UOdinFunctionLibrary::CreateOdinEncoderFromGenerator(this, Room, CaptureAsGenerator);

	// Use the new V2 Audio Pipeline to configure APM (Echo Cancellation, Noise Suppression, etc.)
	if (UOdinPipeline* Pipeline = Encoder->GetOrCreatePipeline())
	{
		
		// Insert APM effect at the start of the pipeline (Index 0)
		const int32 ApmId = Pipeline->InsertApmEffect(0, Encoder->SampleRate, Encoder->bStereo);

		FOdinApmConfig ApmConfig;
		ApmConfig.echo_canceller = true;
		ApmConfig.noise_suppression = EOdinNoiseSuppression::ODIN_NOISE_SUPPRESSION_MODERATE;
		ApmConfig.high_pass_filter = true;
		ApmConfig.gain_controller = EOdinGainControllerVersion::ODIN_GAIN_CONTROLLER_V2;
		ApmConfig.transient_suppressor = true;

		// Apply the configuration
		Pipeline->SetApmConfig(ApmId, ApmConfig);

		// Insert Vad effects next in the pipeline (Index 1)
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

		// Apply the vad configuration
		Pipeline->SetVadConfig(VadId, VadConfig);
	}

	Capture->StartCapturingAudio();
}

void UOdinClientComponent::ConnectToOdin(FGuid PlayerId)
{
	TokenGenerator = UOdinTokenGenerator::ConstructTokenGenerator(this, "<YOUR_ACCESS_KEY>");
	UOdinJsonObject* AuthJson = nullptr;
	TokenGenerator->GenerateRoomToken("TestRoom", "Player", AuthJson, RoomToken);
	if (!AuthJson)
	{
		UE_LOG(LogTemp, Error, TEXT("Generate room token failed, stopping connection process. Please check your Access Key."));
		return;

	}
	UE_LOG(LogTemp, Warning, TEXT("Start connecting with token: %s"), *RoomToken);

	Room = UOdinRoom::ConstructRoom(this);
	// Bind Delegates
	Room->OnRoomPeerJoinedBP.AddUniqueDynamic(this, &UOdinClientComponent::OnPeerJoinedHandler);
	Room->OnRoomJoinedBP.AddUniqueDynamic(this, &UOdinClientComponent::OnRoomJoinSuccessHandler);

	// Add PlayerId to user data
	UOdinJsonObject* UserDataObject = UOdinJsonObject::ConstructJsonObject(this);
	UserDataObject->SetStringField("PlayerId", *PlayerId.ToString());

	// Add user data to authentication object
	AuthJson->SetStringField("user_data", UserDataObject->EncodeJson());

	// Connect with generated room token and initial user data
	bool bSuccess = false;
	Room->ConnectRoom("https://gateway.odin.4players.io", AuthJson->EncodeJson(), bSuccess);
	
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to initiate connection."));
	}
}
