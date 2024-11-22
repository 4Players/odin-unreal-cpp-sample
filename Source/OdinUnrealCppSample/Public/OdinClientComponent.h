// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "OdinTokenGenerator.h"
#include "OdinRoom.h"
#include "CoreMinimal.h"
#include "AudioCapture.h"
#include "Components/ActorComponent.h"
#include "OdinClientComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ODINUNREALCPPSAMPLE_API UOdinClientComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOdinClientComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UOdinTokenGenerator* tokenGenerator;

	FString roomToken;

	UOdinRoom* room;

	FOdinApmSettings apmSettings;

	UFUNCTION()
	void OnPeerJoinedHandler(int64 peerId, FString userId, const TArray<uint8>& userData, UOdinRoom* room);

	UFUNCTION()
	void OnMediaAddedHandler(int64 peerId, UOdinPlaybackMedia* media, UOdinJsonObject* properties, UOdinRoom* room);

	UFUNCTION()
	void OnRoomJoinedHandler(int64 peerId, const TArray<uint8>& roomUserData, UOdinRoom* room);

	FOdinRoomJoinError OnRoomJoinError;
	FOdinRoomJoinSuccess OnRoomJoinSuccess;

	FOdinRoomAddMediaError OnAddMediaError;
	FOdinRoomAddMediaSuccess OnAddMediaSuccess;

	UAudioCapture* capture;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
};
