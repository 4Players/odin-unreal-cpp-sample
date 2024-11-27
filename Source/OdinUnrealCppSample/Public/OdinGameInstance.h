// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OdinGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class ODINUNREALCPPSAMPLE_API UOdinGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	TMap<FGuid, ACharacter*> PlayerCharacters;
	TMap<int64, ACharacter*> OdinPlayerCharacters;
};
