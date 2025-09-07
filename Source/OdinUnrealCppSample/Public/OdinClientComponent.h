// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "OdinTokenGenerator.h"
#include "OdinRoom.h"
#include "CoreMinimal.h"
#include "OdinAudioCapture.h"
#include "Components/ActorComponent.h"
#include "OdinClientComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODINUNREALCPPSAMPLE_API UOdinClientComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOdinClientComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY()
	UOdinTokenGenerator* TokenGenerator;

	UPROPERTY()
	FString RoomToken;

	UPROPERTY()
	UOdinRoom* Room;

	UPROPERTY()
	FOdinApmSettings ApmSettings;

	UFUNCTION()
	void OnRoomJoinSuccessHandler(FString RoomId, const TArray<uint8>& RoomUserData, FString Customer, int64 OwnPeerId,
	                              FString OwnUserId);

	UFUNCTION()
	void OnPeerJoinedHandler(int64 PeerId, FString UserId, const TArray<uint8>& UserData, UOdinRoom* OdinRoom);

	UFUNCTION()
	void OnMediaAddedHandler(int64 PeerId, UOdinPlaybackMedia* Media, UOdinJsonObject* Properties, UOdinRoom* OdinRoom);

	UFUNCTION()
	void OnOdinErrorHandler(int64 ErrorCode);

	FOdinRoomJoinError OnRoomJoinError;
	FOdinRoomJoinSuccess OnRoomJoinSuccess;

	FOdinRoomAddMediaError OnAddMediaError;
	FOdinRoomAddMediaSuccess OnAddMediaSuccess;

	UPROPERTY()
	UOdinAudioCapture* Capture;

public:
	void ConnectToOdin(FGuid PlayerId);
};
