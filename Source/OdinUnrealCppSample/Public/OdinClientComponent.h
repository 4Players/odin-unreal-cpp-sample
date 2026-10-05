// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OdinRoom.h"
#include "OdinClientComponent.generated.h"

class UOdinAudioCapture;
class UOdinDecoder;
class UOdinEncoder;
class UOdinSynthComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ODINUNREALCPPSAMPLE_API UOdinClientComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOdinClientComponent();

	/** Sample-only credential. In production, obtain room tokens from a trusted backend instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Odin|Authentication")
	FString AccessKey;

	/** Connect using AccessKey. The sample character also calls this automatically after receiving its PlayerId. */
	UFUNCTION(BlueprintCallable, Category = "Odin")
	void ConnectToOdin(FGuid PlayerId);

	/** Stop local and remote audio and leave the room. */
	UFUNCTION(BlueprintCallable, Category = "Odin")
	void DisconnectFromOdin();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRoomJoinSuccessHandler(UOdinRoom* OdinRoom, FOdinJoined Data);

	UFUNCTION()
	void OnPeerJoinedHandler(UOdinRoom* OdinRoom, FOdinPeerJoined PeerData);

	UFUNCTION()
	void OnPeerLeftHandler(UOdinRoom* OdinRoom, FOdinPeerLeft PeerData);

	UFUNCTION()
	void OnRoomErrorHandler(UOdinRoom* OdinRoom, FOdinError Data);

private:
	void CleanupLocalAudio();
	void CleanupPeerAudio(int64 PeerId);

	UPROPERTY(Transient)
	UOdinRoom* Room = nullptr;

	UPROPERTY(Transient)
	UOdinAudioCapture* Capture = nullptr;

	UPROPERTY(Transient)
	UOdinEncoder* Encoder = nullptr;

	// Keep the decoders alive independently of synth components owned by remote characters.
	UPROPERTY(Transient)
	TMap<int64, UOdinDecoder*> PeerDecoders;

	UPROPERTY(Transient)
	TMap<int64, UOdinSynthComponent*> PeerSynths;
};
