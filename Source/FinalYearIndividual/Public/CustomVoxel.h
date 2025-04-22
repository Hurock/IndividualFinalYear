#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CustomVoxel.generated.h"

UCLASS()
class ACustomVoxel : public AActor
{
    GENERATED_BODY()

public:
    ACustomVoxel();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    FIntVector Index; // replaces (int, int, int) tuple

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    FVector Position;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float DistToGoal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    float CostToStart;

    UPROPERTY()
    ACustomVoxel* NearestToStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
    bool bVisited;

    UFUNCTION(BlueprintCallable, Category = "Voxel")
    void ResetVoxel(FVector Target);
};
