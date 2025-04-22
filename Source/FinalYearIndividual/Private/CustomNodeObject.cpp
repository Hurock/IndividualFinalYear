#include "CustomNodeObject.h"

void UCustomNodeObject::InitNeighbors(const TArray<FString>& Directions)
{
    Neighbors.Init(Directions);
}

void UCustomNodeObject::AddValidNeighbors(const FString& Key, const TArray<FString>& NewNeighbors)
{
    Neighbors.Valid.FindOrAdd(Key).Neighbors.Append(NewNeighbors);
}

void UCustomNodeObject::AddInvalidNeighbors(const FString& Key, const TArray<FString>& NewNeighbors)
{
    Neighbors.Invalid.FindOrAdd(Key).Neighbors.Append(NewNeighbors);
}

void UCustomNodeObject::SaveNeighbors()
{
    // Flatten to existing structure (if needed for saving)
}

void UCustomNodeObject::LoadNeighbors()
{
    // Rehydrate from flattened (if loaded externally)
}

void UCustomNodeObject::SaveEdges()
{
    EdgeIdx.Empty();
    EdgeDist.Empty();
    for (const FEdgeOctree& Edge : Edges)
    {
        EdgeIdx.Add(Edge.Idx);
        EdgeDist.Add(Edge.Distance);
    }
}

void UCustomNodeObject::LoadEdges()
{
    Edges.Empty();
    for (int32 i = 0; i < EdgeIdx.Num(); ++i)
    {
        Edges.Add(FEdgeOctree(EdgeIdx[i], EdgeDist[i]));
    }
}

void UCustomNodeObject::ResetNode(const FVector& Target)
{
    DistToGoal = FVector::Dist(Target, Position);
    CostToStart = -1.f;
    bVisited = false;
    NearestToStart = nullptr;
}

UCustomNodeObject* UCustomNodeObject::Clone() const
{
    UCustomNodeObject* Copy = NewObject<UCustomNodeObject>(GetOuter());

    Copy->Name = Name;
    Copy->Idx = Idx;
    Copy->Tag = Tag;
    Copy->Parent = Parent;
    Copy->MergeParent = MergeParent;
    Copy->Children = Children;
    Copy->Position = Position;
    Copy->Scale = Scale;
    Copy->DistToGoal = DistToGoal;
    Copy->CostToStart = CostToStart;
    Copy->bVisited = bVisited;

    Copy->Neighbors.Valid = Neighbors.Valid;
    Copy->Neighbors.Invalid = Neighbors.Invalid;

    Copy->Edges = Edges;
    Copy->EdgeIdx = EdgeIdx;
    Copy->EdgeDist = EdgeDist;

    Copy->NearestToStart = nullptr;

    return Copy;
}