// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OdinClientComponent.h"
#include "OdinPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ODINUNREALCPPSAMPLE_API AOdinPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	AOdinPlayerController();

private:
	UPROPERTY()
	UOdinClientComponent* OdinClient;
};
