// Fill out your copyright notice in the Description page of Project Settings.


#include "OdinCharacter.h"
#include "OdinGameInstance.h"
#include "OdinClientComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOdinCharacter::AOdinCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AOdinCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// check if running on any kind of server
	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		// create new guid
		PlayerId = FGuid::NewGuid();

		// add guid and reference to this character to the game instance's player character map
		UOdinGameInstance* GameInstance = Cast<UOdinGameInstance>(UGameplayStatics::GetGameInstance(this));
		if (!GameInstance)
		{
			UE_LOG(LogTemp, Error,
			       TEXT(
				       "AOdinCharacter::BeginPlay: Retrieved invalid Game Instance type, please use UOdinGameInstance as a Game Instance base class."
			       ))
			return;
		}

		GameInstance->PlayerCharacters.Add(PlayerId, this);

		UE_LOG(LogTemp, Warning, TEXT("Created PlayerId: %s"), *PlayerId.ToString());

		// if this character is also controlled locally we want to start the routine to join the Odin Room
		if (IsLocallyControlled())
		{
			UOdinClientComponent* OdinClientComponent = UGameplayStatics::GetPlayerControllerFromID(this, 0)->
				GetComponentByClass<UOdinClientComponent>();
			if (OdinClientComponent)
			{
				OdinClientComponent->ConnectToOdin(PlayerId);
			}
		}
	}
}

void AOdinCharacter::OnRep_PlayerId()
{
	UOdinGameInstance* GameInstance = Cast<UOdinGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error,
		       TEXT(
			       "AOdinCharacter::OnRep_PlayerId: Retrieved invalid Game Instance type, please use UOdinGameInstance as a Game Instance base class."
		       ))
		return;
	}

	GameInstance->PlayerCharacters.Add(PlayerId, this);

	UE_LOG(LogTemp, Warning, TEXT("Replicated PlayerId: %s"), *PlayerId.ToString());

	if (IsLocallyControlled())
	{
		UOdinClientComponent* OdinClientComponent = UGameplayStatics::GetPlayerControllerFromID(this, 0)->
			GetComponentByClass<UOdinClientComponent>();
		if (OdinClientComponent)
		{
			OdinClientComponent->ConnectToOdin(PlayerId);
		}
	}
}

void AOdinCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOdinCharacter, PlayerId);
}
