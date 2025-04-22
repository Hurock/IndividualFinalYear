#include "DetectCollisionVoxelComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UDetectCollisionVoxelComponent::UDetectCollisionVoxelComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDetectCollisionVoxelComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner())
    {
        GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UDetectCollisionVoxelComponent::OnBeginOverlap);
    }
}

void UDetectCollisionVoxelComponent::OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!OtherActor || !OtherActor->ActorHasTag("Obstacle")) return;

    OverlappedActor->Tags.AddUnique("Invalid");
}
