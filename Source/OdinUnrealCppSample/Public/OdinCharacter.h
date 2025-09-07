// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OdinCharacter.generated.h"

UCLASS()
class ODINUNREALCPPSAMPLE_API AOdinCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AOdinCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerId)
	FGuid PlayerId;

	UFUNCTION()
	void OnRep_PlayerId();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
