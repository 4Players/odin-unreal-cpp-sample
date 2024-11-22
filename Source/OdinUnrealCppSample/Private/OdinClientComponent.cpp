// Fill out your copyright notice in the Description page of Project Settings.

#include "OdinClientComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "OdinSynthComponent.h"
#include "AudioCaptureBlueprintLibrary.h"
#include "OdinFunctionLibrary.h"

// Sets default values for this component's properties
UOdinClientComponent::UOdinClientComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOdinClientComponent::BeginPlay()
{
	Super::BeginPlay();

	tokenGenerator = UOdinTokenGenerator::ConstructTokenGenerator(this, "AQGEYTtGuFdlq6Msk+bO9ki6dDJ+fG8UmjfZD+VZOuUt");

	roomToken = tokenGenerator->GenerateRoomToken("Test", "Player", EOdinTokenAudience::Default);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *roomToken);

	apmSettings = FOdinApmSettings();

	apmSettings.bVoiceActivityDetection = true;
	apmSettings.fVadAttackProbability = 0.9;
	apmSettings.fVadReleaseProbability = 0.8;
	apmSettings.bEnableVolumeGate = false;
	apmSettings.fVolumeGateAttackLoudness = -90.0;
	apmSettings.fVolumeGateReleaseLoudness = -90.0;
	apmSettings.bHighPassFilter = false;
	apmSettings.bPreAmplifier = false;
	apmSettings.noise_suppression_level = EOdinNoiseSuppressionLevel::OdinNS_Moderate;
	apmSettings.bTransientSuppresor = false;
	apmSettings.bEchoCanceller = true;

	room = UOdinRoom::ConstructRoom(this, apmSettings);
	
	room->onPeerJoined.AddUniqueDynamic(this, &UOdinClientComponent::OnPeerJoinedHandler);
	room->onMediaAdded.AddUniqueDynamic(this, &UOdinClientComponent::OnMediaAddedHandler);
	OnRoomJoinSuccess.BindUFunction(this, TEXT("OnRoomJoinedHandler")); 
	OnRoomJoinError.BindUFunction(this, TEXT("OnOdinErrorHandler"));

	TArray<uint8> userData = { 0 };

	UOdinRoomJoin* Action = UOdinRoomJoin::JoinRoom(this, room, TEXT("https://gateway.odin.4players.io"), roomToken, userData, FVector(0, 0, 0), OnRoomJoinError, OnRoomJoinSuccess);
	Action->Activate();
}

void UOdinClientComponent::OnPeerJoinedHandler(int64 peerId, FString userId, const TArray<uint8>& userData, UOdinRoom* joinedRoom)
{
	UE_LOG(LogTemp, Warning, TEXT("Peer %s joined"), *userId);
}

void UOdinClientComponent::OnMediaAddedHandler(int64 peerId, UOdinPlaybackMedia* media, UOdinJsonObject* properties, UOdinRoom* addedInRoom)
{
	ACharacter* player = UGameplayStatics::GetPlayerCharacter(this, 0);
	UActorComponent* comp = player->AddComponentByClass(UOdinSynthComponent::StaticClass(), false, FTransform::Identity, false);
	UOdinSynthComponent* synth = StaticCast<UOdinSynthComponent*>(comp);

	synth->Odin_AssignSynthToMedia(media);

	synth->Activate();

	UE_LOG(LogTemp, Warning, TEXT("Odin Synth Added"));
}

void UOdinClientComponent::OnRoomJoinedHandler(int64 peerId, const TArray<uint8>& roomUserData, UOdinRoom* joinedRoom)
{
	UE_LOG(LogTemp, Warning, TEXT("Joined Room"));

	capture = UOdinFunctionLibrary::CreateOdinAudioCapture(this);

	// cast pointer to capture to UAudioGenerator for Odin_CreateMedia
	UAudioGenerator* captureAsGenerator = (UAudioGenerator*)capture;

	auto media = UOdinFunctionLibrary::Odin_CreateMedia(captureAsGenerator);


	OnAddMediaError.BindUFunction(this, TEXT("OnOdinErrorHandler"));

	UOdinRoomAddMedia* Action = UOdinRoomAddMedia::AddMedia(this, room, media, OnAddMediaError, OnAddMediaSuccess)
	Action->Activate();

	capture->StartCapturingAudio();
}

void UOdinClientComponent::OnOdinErrorHandler(int64 errorCode)
{
	FString errorString = UOdinFunctionLibrary::FormatError(errorCode, true);
	UE_LOG(LogTemp, Error, TEXT("%S"), *errorString);
}



// Called every frame
void UOdinClientComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

