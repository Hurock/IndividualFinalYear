#include "AOctreeActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "CustomNodeObject.h"
#include "DrawDebugHelpers.h"
#include "NodeDataMerged.h"

AOctreeActor::AOctreeActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AOctreeActor::BeginPlay()
{
    Super::BeginPlay();

    ZBound = 2 * Bound;

    Data = NewObject<UNodeDataMerged>(this, UNodeDataMerged::StaticClass());
    // TODO: hook this with your AstarOctree equivalent

    // You can simulate scene name with level name
    ScenePath = FString::Printf(TEXT("/Game/Data/%s%.0f"), *GetWorld()->GetMapName(), MinSize);

    if (bLoadFromFile)
    {
        // TODO: Load from file system - Asset registry, binary, etc.
        UE_LOG(LogTemp, Warning, TEXT("TODO: Load node data from %s"), *ScenePath);
        Task = TEXT("finished");
        Tags.Add(FName("Finished"));
    }
    else
    {
        CreateOctree();
    }
    
}

void AOctreeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (Task == TEXT("build"))
    {
        T0 = GetWorld()->GetTimeSeconds();
        while (ToSplit.Num() > 0)
        {
            UCustomNodeObject* Node = ToSplit[0];
            ToSplit.RemoveAt(0);
            SplitNode(Node);
        }
        Task = TEXT("prune");
    }
    else if (Task == TEXT("prune"))
    {
        PruneOctree();

        double GenTime = GetWorld()->GetTimeSeconds() - T0;
        UE_LOG(LogTemp, Log, TEXT("OctTree built in %f s"), GenTime);
        UE_LOG(LogTemp, Log, TEXT("Valid Node Numbers = %d"), Data->ValidNodes.Num());
        Task = TEXT("graph");
    }
    else if (Task == TEXT("graph"))
    {
        BuildGraph();
        Task = TEXT("finished");
        Tags.Add(FName("Finished"));

        if (Task == "finished" && bDraw)
        {
            DrawDebugOctree();
        }
    }
    else
    {
        // TODO: Dynamic update (when MapGenerator exists, etc.)
    }

    
}

void AOctreeActor::CreateOctree()
{
    UE_LOG(LogTemp, Log, TEXT("Creating new octree"));

    ToSplit.Empty();

    UCustomNodeObject* RootNode = NewObject<UCustomNodeObject>(this);
    RootNode->Position = FVector(ZBound / 2, ZBound / 2.f, ZBound /2);
    RootNode->Scale = FVector(2 * Bound, ZBound, 2 * Bound);
    RootNode->Idx = TEXT("0");
    RootNode->Tag = TEXT("Valid");

    // Init neighbor maps
    RootNode->InitNeighbors();

    BuildOctree(RootNode);

    Task = TEXT("build");
}

void AOctreeActor::UpdateOctree()
{

}

void AOctreeActor::BuildOctree(UCustomNodeObject* NewNode)
{
    Data->ValidNodes.Add(NewNode->Idx, NewNode);

    // TODO: Implement IsEmpty (CollisionCheck)
    if (!IsEmpty(NewNode->Position, NewNode->Scale))
    {
        ToSplit.Add(NewNode);
    }
    else
    {
        for (const auto& Entry : NewNode->Neighbors.Valid)
        {
            for (const FString& Neigh : Entry.Value.Neighbors)
            {
                TransitionsAdd.Add(TPair<FString, FString>(NewNode->Idx, Neigh));
            }
        }
    }
}

bool AOctreeActor::IsEmpty(FVector Position, FVector Scale)
{
    UWorld* World = GetWorld();
    if (!World) return true;

    // Half extents, like Unity's BoxCast
    FVector HalfExtent = Scale / 2.0f;

    // Rotation — if your boxes are axis-aligned, this stays zero
    FQuat Rotation = FQuat::Identity;

    // Set up collision shape
    FCollisionShape CollisionShape = FCollisionShape::MakeBox(HalfExtent);

    // Define collision query params
    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;
    QueryParams.AddIgnoredActor(this); // Ignore self

    // Perform overlap test
    bool bHit = World->OverlapBlockingTestByChannel(
        Position,
        Rotation,
        ECC_WorldStatic, // Or use your custom trace channel
        CollisionShape,
        QueryParams
    );

    return !bHit; // if it hits, it's NOT empty
}

FVector AOctreeActor::CenterToCorner(FVector Center, FVector Scale) const
{
    return Center - Scale / 2.f;
}

FVector AOctreeActor::CornerToCenter(FVector Corner, FVector Scale) const
{
    return Corner + Scale / 2.f;
}

int32 GCD(int32 A, int32 B)
{
    while (A != 0 && B != 0)
    {
        if (A > B)
            A %= B;
        else
            B %= A;
    }
    return A | B;
}

FVector Center2corner(FVector center, FVector scale)
{
    return center - scale / 2;
}

FVector Corner2center(FVector corner, FVector scale)
{
    return corner + scale / 2;
}


void AOctreeActor::SplitNode(UCustomNodeObject* Node)
{
    if (Node->Tag == "Valid")
    {
        Node->Tag = "Node";
        Data->ValidNodes.Remove(Node->Idx);
        Data->Cells.Add(Node->Idx, Node);
    }

    FVector CurrentScale = Node->Scale;

    if (CurrentScale.X > MinSize || CurrentScale.Y > MinSize || CurrentScale.Z > MinSize)
    {
        // Remove transitions
        for (const auto& Pair : Node->Neighbors.Valid)
        {
            for (const FString& Neigh : Pair.Value.Neighbors)
            {
                TransitionsRemove.Add(TPair<FString, FString>(Node->Idx, Neigh));
            }
        }

        auto [NewScale, NewCenters] = GetNewCenters(Node);
        TArray<UCustomNodeObject*> NewChildren;

        for (const FVector& Pos : NewCenters)
        {
            UCustomNodeObject* Child = NewObject<UCustomNodeObject>(this);
            Child->InitNeighbors();
            Child->Position = Pos;
            Child->Scale = NewScale;
            Child->Idx = FString::FromInt(GlobalId++);
            Child->Name = Child->Idx;
            Child->Tag = "Valid";
            Child->Parent = Node->Idx;

            if (Task == "finished")
            {
                Child->MergeParent = Node->Idx;
            }

            if (!Node->Children.Contains(Child->Idx))
            {
                Node->Children.Add(Child->Idx);
            }

            NewChildren.Add(Child);
        }

        // Update neighbors
        Data->UpdateNeighborsOnSplit(Node, NewChildren);

        for (UCustomNodeObject* Child : NewChildren)
        {
            auto [ValidNeighbors, InvalidNeighbors] = Data->ComputeChildNeighbors(Node, Child);

            Child->Neighbors.Valid.Empty();
            for (const auto& Pair : ValidNeighbors)
            {
                FNeighborList Wrapped;
                Wrapped.Neighbors = Pair.Value;
                Child->Neighbors.Valid.Add(Pair.Key, Wrapped);
            }

            Child->Neighbors.Invalid.Empty();
            for (const auto& Pair : InvalidNeighbors)
            {
                FNeighborList Wrapped;
                Wrapped.Neighbors = Pair.Value;
                Child->Neighbors.Invalid.Add(Pair.Key, Wrapped);
            }

            for (UCustomNodeObject* OtherChild : NewChildren)
            {
                if (OtherChild != Child)
                {
                    FString Dir = Data->IsNeighbor(Child, OtherChild);
                    if (!Dir.IsEmpty())
                    {
                        Child->Neighbors.Valid[Dir].Neighbors.Add(OtherChild->Idx);
                    }
                }
            }
            BuildOctree(Child);
        }
    }
    else
    {
        // Mark node as invalid
        Node->Tag = "Invalid";
        Data->ValidNodes.Remove(Node->Idx);
        Data->InvalidNodes.Add(Node->Idx, Node);
        Data->UpdateNeighborsOnInvalid(Node);

        for (const auto& Pair : Node->Neighbors.Valid)
        {
            for (const FString& Neigh : Pair.Value.Neighbors)
            {
                TransitionsRemove.Add(TPair<FString, FString>(Node->Idx, Neigh));
            }
        }
    }
}

TPair<FVector, TArray<FVector>> AOctreeActor::GetNewCenters(UCustomNodeObject* Node)
{
    TArray<FVector> NewCenters;
    FVector NewScale;
    FVector Center = Node->Position;
    FVector Scale = Node->Scale;

    if (Scale.X == Scale.Y && Scale.X == Scale.Z)
    {
        NewScale = Scale / 2.0f;
        FVector Corner = Center2corner(Center, Scale);

        for (int32 dx = 0; dx <= 1; ++dx)
            for (int32 dy = 0; dy <= 1; ++dy)
                for (int32 dz = 0; dz <= 1; ++dz)
                {
                    FVector Offset = FVector(dx * NewScale.X, dy * NewScale.Y, dz * NewScale.Z);
                    NewCenters.Add(Corner2center(Corner + Offset, NewScale));
                }
    }
    else
    {
        int32 X = FMath::FloorToInt(Scale.X / MinSize);
        int32 Y = FMath::FloorToInt(Scale.Y / MinSize);
        int32 Z = FMath::FloorToInt(Scale.Z / MinSize);

        float MinUnit = GCD(GCD(X, Y), Z) * MinSize;
        NewScale = FVector(MinUnit);

        FVector Corner = Center2corner(Center, Scale);
        for (int32 i = 0; i < Scale.X / MinUnit; ++i)
            for (int32 j = 0; j < Scale.Y / MinUnit; ++j)
                for (int32 k = 0; k < Scale.Z / MinUnit; ++k)
                {
                    FVector Offset(i * MinUnit, j * MinUnit, k * MinUnit);
                    NewCenters.Add(Corner2center(Corner + Offset, NewScale));
                }
    }

    return TPair<FVector, TArray<FVector>>(NewScale, NewCenters);
}

void AOctreeActor::PruneOctree()
{
    if (bPrune)
    {
        TArray<FString> MergeTargets;

        for (const auto& Pair : Data->Cells)
        {
            const FString& ParentId = Pair.Key;
            UCustomNodeObject* Parent = Pair.Value;

            if (Parent->Children.Num() == 0)
                continue;

            bool bCanMerge = true;

            for (const FString& ChildId : Parent->Children)
            {
                if (!Data->InvalidNodes.Contains(ChildId))
                {
                    bCanMerge = false;
                    break;
                }
            }

            if (bCanMerge)
            {
                MergeTargets.Add(ParentId);
            }
        }

        for (const FString& ParentId : MergeTargets)
        {
            UCustomNodeObject* Parent = Data->Cells[ParentId];

            for (const FString& ChildId : Parent->Children)
            {
                Data->InvalidNodes.Remove(ChildId);
            }

            Parent->Tag = "Valid";
            Data->ValidNodes.Add(Parent->Idx, Parent);

            Data->UpdateNeighborsOnMerge(Parent);

            for (const auto& Pair : Parent->Neighbors.Valid)
            {
                for (const FString& Neigh : Pair.Value.Neighbors)
                {
                    TransitionsAdd.Add(TPair<FString, FString>(Parent->Idx, Neigh));
                }
            }
        }

        MergeNeighbors(MergeTargets);
    }
}


void AOctreeActor::BuildGraph()
{
    UE_LOG(LogTemp, Log, TEXT("Building octree graph"));

    // 1. Remove transitions
    for (const TPair<FString, FString>& Pair : TransitionsRemove)
    {
        UCustomNodeObject* A = Data->FindNode(Pair.Key);
        UCustomNodeObject* B = Data->FindNode(Pair.Value);

        if (A && B)
        {
            for (int32 i = 0; i < A->Edges.Num(); ++i)
            {
                if (A->Edges[i].Idx == B->Idx)
                {
                    A->Edges.RemoveAt(i);
                    break;
                }
            }

            for (int32 i = 0; i < B->Edges.Num(); ++i)
            {
                if (B->Edges[i].Idx == A->Idx)
                {
                    B->Edges.RemoveAt(i);
                    break;
                }
            }
        }
    }

    // 2. Add transitions
    for (const TPair<FString, FString>& Pair : TransitionsAdd)
    {
        UCustomNodeObject* A = Data->FindNode(Pair.Key);
        UCustomNodeObject* B = Data->FindNode(Pair.Value);

        if (A && B)
        {
            float Distance = FVector::Dist(A->Position, B->Position);
            A->Edges.Add(FEdgeOctree(B->Idx, Distance));
            B->Edges.Add(FEdgeOctree(A->Idx, Distance));
        }
    }

    // 3. Save edges for serialization/debug
    for (const auto& Pair : Data->ValidNodes)
    {
        UCustomNodeObject* Node = Pair.Value;
        Node->SaveEdges();
    }

    TransitionsAdd.Empty();
    TransitionsRemove.Empty();

    UE_LOG(LogTemp, Log, TEXT("Graph construction complete."));
}

void AOctreeActor::DrawDebugOctree()
{
    UWorld* World = GetWorld();
    if (!World || !bDraw) return;

    UE_LOG(LogTemp, Warning, TEXT("DRAWING OCTREE..."));

    FColor ValidColor = FColor::Green;
    FColor InvalidColor = FColor::Red;

    // 1. Draw Valid nodes
    if (bDrawValid)
    {
        for (const auto& Pair : Data->ValidNodes)
        {
            UCustomNodeObject* Node = Pair.Value;
            DrawDebugBox(World, Node->Position, Node->Scale / 2.0f, ValidColor, true, -1.0f, 0, 2.0f);
        }
    }
    

    // 2. Draw Invalid nodes
    if (bDrawInvalid)
    {
        for (const auto& Pair : Data->InvalidNodes)
        {
            UCustomNodeObject* Node = Pair.Value;
            DrawDebugBox(World, Node->Position, Node->Scale / 2.0f, InvalidColor, true, -1.0f, 0, 2.0f);
        }
    }
    

    // 3. Draw graph edges (optional)
    for (const auto& Pair : Data->ValidNodes)
    {
        UCustomNodeObject* Node = Pair.Value;
        for (const FEdgeOctree& Edge : Node->Edges)
        {
            UCustomNodeObject* Neighbor = Data->FindNode(Edge.Idx);
            if (Neighbor)
            {
                DrawDebugLine(World, Node->Position, Neighbor->Position, FColor::Blue, false, -1.0f, 0, 1.0f);
            }
        }
    }
}

void AOctreeActor::MergeNeighbors(TArray<FString>& MergeTargets)
{
    int32 MergedCount = 0;

    for (const FString& ParentId : MergeTargets)
    {
        if (!Data->Cells.Contains(ParentId)) continue;

        UCustomNodeObject* Parent = Data->Cells[ParentId];

        if (Parent->Children.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Parent %s has no children, skipping merge."), *ParentId);
            continue;
        }
        // Check if parent exists and is in Data->Cells
        bool bAllChildrenInvalid = true;

        for (const FString& ChildId : Parent->Children)
        {
            if (Data->InvalidNodes.Contains(ChildId))
            {
                bAllChildrenInvalid = false;
                UE_LOG(LogTemp, Warning, TEXT("Cannot merge Parent %s because Child %s is NOT invalid"), *ParentId, *ChildId);
                break;
            }
        }

        if (!bAllChildrenInvalid) continue;

        for (const FString& ChildId : Parent->Children)
        {
            Data->InvalidNodes.Remove(ChildId);
        }

        Parent->Tag = "Valid";
        Data->ValidNodes.Add(Parent->Idx, Parent);
        Data->UpdateNeighborsOnMerge(Parent);

        // Add back graph transitions
        for (const auto& Pair : Parent->Neighbors.Valid)
        {
            for (const FString& Neigh : Pair.Value.Neighbors)
            {
                TransitionsAdd.Add(TPair<FString, FString>(Parent->Idx, Neigh));
            }
        }

        // Clear children after merge
        Parent->Children.Empty();

        ++MergedCount;

        //UE_LOG(LogTemp, Log, TEXT("Parent %s merged successfully."), *ParentId);
    }
    UE_LOG(LogTemp, Log, TEXT("MergeNeighbors: Merged %d parent nodes."), MergedCount);
}