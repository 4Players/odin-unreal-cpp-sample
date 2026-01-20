// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "OdinTokenGenerator.h"
#include "OdinRoom.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdinAudio/OdinAudioCapture.h"
#include "OdinClientComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODINUNREALCPPSAMPLE_API UOdinClientComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOdinClientComponent();

protected:
	UPROPERTY()
	UOdinTokenGenerator* TokenGenerator;

	UPROPERTY()
	FString RoomToken;

	UPROPERTY()
	UOdinRoom* Room;

	UFUNCTION()
	void OnRoomJoinSuccessHandler(UOdinRoom* OdinRoom,  FOdinJoined Data);

	UFUNCTION()
	void OnPeerJoinedHandler(UOdinRoom* OdinRoom, FOdinPeerJoined PeerData);

	UFUNCTION()
	void OnOdinErrorHandler(int64 ErrorCode);

	UPROPERTY()
	UOdinAudioCapture* Capture;
	
	UPROPERTY()
	UOdinEncoder* Encoder;

public:
	void ConnectToOdin(FGuid PlayerId);
};
