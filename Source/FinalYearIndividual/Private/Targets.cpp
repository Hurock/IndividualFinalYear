// Fill out your copyright notice in the Description page of Project Settings.


#include "Targets.h"

// Sets default values
ATargets::ATargets()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATargets::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATargets::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

