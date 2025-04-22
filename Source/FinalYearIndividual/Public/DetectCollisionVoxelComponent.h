#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DetectCollisionVoxelComponent.generated.h"

class AVoxelManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UDetectCollisionVoxelComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDetectCollisionVoxelComponent();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
};

