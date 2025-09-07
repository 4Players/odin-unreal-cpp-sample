// Fill out your copyright notice in the Description page of Project Settings.


#include "OdinPlayerController.h"

AOdinPlayerController::AOdinPlayerController()
{
	OdinClient = CreateDefaultSubobject<UOdinClientComponent>(TEXT("OdinClient"));
}