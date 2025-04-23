#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AOctreeActor.generated.h"

class UNodeDataMerged;
class UCustomNodeObject;

UCLASS()
class AOctreeActor : public AActor
{
    GENERATED_BODY()

public:
    AOctreeActor();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Configurable in editor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    float MinSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    float Bound = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bPrune = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bDraw = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bDrawValid = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bDrawInvalid = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bLoad = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Octree")
    bool bLoadFromFile = false;

    // Placeholder for node visuals
    UPROPERTY(EditAnywhere, Category = "Octree")
    TSubclassOf<AActor> ValidNodeVisual;

    UPROPERTY(EditAnywhere, Category = "Octree")
    TSubclassOf<AActor> InvalidNodeVisual;

private:
    float ZBound = 0.f;
    int32 GlobalId = 1;
    FString Task;

    TArray<UCustomNodeObject*> ToSplit;
    TArray<UCustomNodeObject*> ToRepair;
    TArray<TPair<FString, FString>> TransitionsAdd;
    TArray<TPair<FString, FString>> TransitionsRemove;

    double T0 = 0;
    FString ScenePath;

    UPROPERTY()
    UNodeDataMerged* Data;

    TArray<FString> Directions = { "left", "right", "up", "down", "forward", "backward" };

    bool bTested = false;

    void CreateOctree();
    void UpdateOctree();
    void BuildGraph();
    void PruneOctree();

    void SplitNode(UCustomNodeObject* Node);
    void BuildOctree(UCustomNodeObject* NewNode);

    // Helper methods
    FVector CenterToCorner(FVector Center, FVector Scale) const;
    FVector CornerToCenter(FVector Corner, FVector Scale) const;
    TPair<FVector, TArray<FVector>> GetNewCenters(UCustomNodeObject* Node);

    void DrawDebugOctree();
    bool IsEmpty(FVector Position, FVector Scale);
    void MergeNeighbors(TArray<FString>& MergeTargets);
};