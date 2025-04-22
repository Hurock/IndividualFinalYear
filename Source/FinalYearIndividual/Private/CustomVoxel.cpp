#include "CustomVoxel.h"

ACustomVoxel::ACustomVoxel()
{
    PrimaryActorTick.bCanEverTick = false;
    DistToGoal = 0.f;
    CostToStart = -1.f;
    bVisited = false;
    NearestToStart = nullptr;
}

void ACustomVoxel::ResetVoxel(FVector Target)
{
    DistToGoal = FVector::Dist(Target, Position);
    CostToStart = -1.f;
    bVisited = false;
    NearestToStart = nullptr;
}
