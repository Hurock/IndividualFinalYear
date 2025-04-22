#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CustomNodeObject.generated.h"

USTRUCT(BlueprintType)
struct FNeighborList
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> Neighbors;
};

USTRUCT(BlueprintType)
struct FNeighborMap
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<FString, FNeighborList> Valid;

    UPROPERTY()
    TMap<FString, FNeighborList> Invalid;

    void Init(const TArray<FString>& Directions)
    {
        for (const FString& Dir : Directions)
        {
            Valid.Add(Dir, FNeighborList());
            Invalid.Add(Dir, FNeighborList());
        }
    }
};

USTRUCT(BlueprintType)
struct FEdgeOctree
{
    GENERATED_BODY()

    UPROPERTY()
    FString Idx;

    UPROPERTY()
    float Distance;

    FEdgeOctree() : Idx(""), Distance(0.f) {}
    FEdgeOctree(const FString& InIdx, float InDistance) : Idx(InIdx), Distance(InDistance) {}

    FORCEINLINE bool operator==(const FEdgeOctree& Other) const
    {
        return Idx == Other.Idx && FMath::IsNearlyEqual(Distance, Other.Distance);
    }

};

UCLASS(BlueprintType)
class UCustomNodeObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString Name;

    UPROPERTY()
    FString Idx;

    UPROPERTY()
    FString Parent;

    UPROPERTY()
    FString MergeParent;

    UPROPERTY()
    FString Tag;

    UPROPERTY()
    FVector Position;

    UPROPERTY()
    FVector Scale;

    UPROPERTY()
    float DistToGoal;

    UPROPERTY()
    float CostToStart;

    UPROPERTY()
    bool bVisited;

    UPROPERTY()
    UCustomNodeObject* NearestToStart;

    UPROPERTY()
    TArray<FString> Children;

    UPROPERTY()
    FNeighborMap Neighbors;

    UPROPERTY()
    TArray<FEdgeOctree> Edges;

    // Serialization support
    UPROPERTY()
    TArray<FString> EdgeIdx;

    UPROPERTY()
    TArray<float> EdgeDist;

    UPROPERTY()
    bool Visited;

    void InitNeighbors(const TArray<FString>& Directions = { "up", "down", "left", "right", "forward", "backward" });

    void AddValidNeighbors(const FString& Key, const TArray<FString>& NewNeighbors);
    void AddInvalidNeighbors(const FString& Key, const TArray<FString>& NewNeighbors);

    void SaveNeighbors();
    void LoadNeighbors();

    void SaveEdges();
    void LoadEdges();

    void ResetNode(const FVector& Target);
    UCustomNodeObject* Clone() const;
};
