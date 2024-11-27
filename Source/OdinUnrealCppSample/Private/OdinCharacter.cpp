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


	if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
	{
		PlayerId = FGuid::NewGuid();

		UOdinGameInstance* gameInstance = StaticCast<UOdinGameInstance*>(UGameplayStatics::GetGameInstance(this));

		gameInstance->PlayerCharacters.Add(PlayerId, this);

		UE_LOG(LogTemp, Warning, TEXT("Created PlayerId: %s"), *PlayerId.ToString());

		if (IsLocallyControlled())
		{
			UGameplayStatics::GetPlayerControllerFromID(this, 0)->GetComponentByClass<UOdinClientComponent>()->ConnectToOdin(PlayerId);
		}
	}
}

// Called every frame
void AOdinCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AOdinCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AOdinCharacter::OnRep_PlayerId()
{
	UOdinGameInstance* gameInstance = StaticCast<UOdinGameInstance*>(UGameplayStatics::GetGameInstance(this));

	gameInstance->PlayerCharacters.Add(PlayerId, this);

	UE_LOG(LogTemp, Warning, TEXT("Replicated PlayerId: %s"), *PlayerId.ToString());

	if (IsLocallyControlled())
	{
		UGameplayStatics::GetPlayerControllerFromID(this, 0)->GetComponentByClass<UOdinClientComponent>()->ConnectToOdin(PlayerId);
	}
}

void AOdinCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOdinCharacter, PlayerId);
}

