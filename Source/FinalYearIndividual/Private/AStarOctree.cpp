// AstarOctree.cpp
#include "AstarOctree.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "CustomNodeObject.h"
#include "NodeDataMerged.h"
#include "AOctreeActor.h"
#include "Targets.h"
//#include "Funnel.h"

AAstarOctree::AAstarOctree()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAstarOctree::BeginPlay()
{
    Super::BeginPlay();

    bDone = false;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargets::StaticClass(), FoundActors);

    if (FoundActors.Num() >= 2)
    {
        Start = FoundActors[0]->GetActorLocation();
        Target = FoundActors[1]->GetActorLocation();

        StartNode = NewObject<UCustomNodeObject>(this, UCustomNodeObject::StaticClass(), TEXT("StartNode"));
        StartNode->Name = TEXT("StartNode");
        StartNode->Position = Start;

        TargetNode = NewObject<UCustomNodeObject>(this, UCustomNodeObject::StaticClass(), TEXT("TargetNode"));
        TargetNode->Name = TEXT("TargetNode");
        TargetNode->Position = Target;

        // Debug log
        UE_LOG(LogTemp, Warning, TEXT("Start: %s | Pos: %s"), *StartNode->Name, *StartNode->Position.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Target: %s | Pos: %s"), *TargetNode->Name, *TargetNode->Position.ToString());

    }

    PathDrawer = GetWorld()->SpawnActor<AActor>(AActor::StaticClass());
    PathDrawer->SetActorLabel(TEXT("PathDrawer"));
}

void AAstarOctree::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bDone && this->ActorHasTag(FName("Finished")))
    {
        bDone = true;

        FString Folder = FPaths::ProjectDir() + "/TestPoints/" + GetWorld()->GetMapName();

        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*Folder) && bReadFromFile)
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*Folder);
            CreateRandomPositions(1000, Folder);
        }

        if (bReadFromFile)
        {
            TestRandomPositions(Folder);
        }
        else
        {
            AStarPath(Start, Target);
        }

        FString Path = GetWorld()->GetMapName() + FString::FromInt(Cast<AOctreeActor>(GetComponentByClass(AOctreeActor::StaticClass()))->MinSize);
        if (!Cast<AOctreeActor>(GetComponentByClass(AOctreeActor::StaticClass()))->bLoad)
        {
            UE_LOG(LogTemp, Warning, TEXT("Saving data to file %s..."), *Path);
            double t0 = FPlatformTime::Seconds();
            //Data->SaveData(Path);
            double dt = FPlatformTime::Seconds() - t0;
            UE_LOG(LogTemp, Warning, TEXT("Saved in %.3f ms"), dt * 1000);
        }
    }
    else if (bDone)
    {
        if (bMove && DistanceAlongPath < PathIdx.Last().Get<2>() + 1)
        {
            if (FMath::IsNearlyZero(DistanceAlongPath))
            {
                AActor* Player = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Start, FRotator::ZeroRotator);
                Player->SetActorScale3D(FVector(0.5f));
                Player->SetActorLabel(TEXT("Player"));
            }
            MoveAlongPath();
            DistanceAlongPath += 2 * DeltaTime;
        }
    }
}

struct FNodeWithPriority
{
    UCustomNodeObject* Node;
    float Priority;

    bool operator<(const FNodeWithPriority& Other) const
    {
        return Priority > Other.Priority; // min-heap
    }
};

void AAstarOctree::DrawPath(const TArray<FVector>& Path, FColor Color)
{
    if (bDraw) return;

    for (int32 i = 0; i < Path.Num() - 1; ++i)
    {
        const FVector& StartPoint = Path[i];
        const FVector& EndPoint = Path[i + 1];

        DrawDebugLine(
            GetWorld(),
            StartPoint,
            EndPoint,
            Color,
            false,
            10.0f,
            0,
            5.0f
        );
    }
}

TTuple<int32, float, double> AAstarOctree::AStarPath(const FVector& InStart, const FVector& InTarget)
{
    NumVisited = 0;
    TempEdges.Empty();
    double StartTime = FPlatformTime::Seconds();

    StartNode = NewObject<UCustomNodeObject>();
    StartNode->Name = TEXT("Start");
    StartNode->Idx = TEXT("Start");
    StartNode->Position = InStart;
    InsertTempNode(StartNode);

    TargetNode = NewObject<UCustomNodeObject>();
    TargetNode->Name = TEXT("Target");
    TargetNode->Idx = TEXT("Target");
    TargetNode->Position = InTarget;
    InsertTempNode(TargetNode);

    TArray<UCustomNodeObject*> ShortestPath;
    TArray<FVector> PathPositions;
    float PathLen = 0.0f;

    if (StartIdx != TargetIdx)
    {
        for (auto& Elem : Data->ValidNodes)
        {
            Elem.Value->ResetNode(InTarget);
        }
        AStarSearch();

        UCustomNodeObject* Current = TargetNode;
        while (Current->NearestToStart != StartNode)
        {
            ShortestPath.Add(Current);
            Current = Current->NearestToStart;
        }
        ShortestPath.Add(Current);
        ShortestPath.Add(StartNode);
        Algo::Reverse(ShortestPath);

        for (auto* Node : ShortestPath)
        {
            PathPositions.Add(Node->Position);
            if (Node->Idx != TEXT("Start") && Node->Idx != TEXT("Target"))
            {
                TArray<FString> Parts;
                Node->Idx.ParseIntoArray(Parts, TEXT("&"));
                for (const FString& Part : Parts)
                {
                    if (!Data->PathCells.Contains(Part))
                        Data->PathCells.Add(Part);
                }
            }
        }

        if (bDraw)
            DrawPath(PathPositions, FColor::Red);

        if (bFunnel)
        {
            // Funnel implementation pending
        }

        if (bDraw)
            DrawPath(PathPositions, FColor::Yellow);

        if (bPrune)
        {
            PathPositions = PrunePath(PathPositions);
        }

        if (bDraw)
            DrawPath(PathPositions, FColor::Green);

        if (bMove)
        {
            ComputePathIdx2(PathPositions);
        }

        PathLen = PathLength(PathPositions);
    }
    else
    {
        ShortestPath = { StartNode, TargetNode };
        Data->PathCells.Add(StartIdx);
        PathLen = FVector::Dist(StartNode->Position, TargetNode->Position);
    }

    for (const auto& Elem : TempEdges)
    {
        const FString& Key = Elem.Key;
        for (const auto& Val : Elem.Value)
        {
            Data->ValidNodes[Key]->Edges.Remove(FEdgeOctree{ Val.Key, Val.Value });
        }
    }
    TempEdges.Empty();

    double TotalTime = FPlatformTime::Seconds() - StartTime;
    return MakeTuple(NumVisited, PathLen, TotalTime);
}

void AAstarOctree::AStarSearch()
{

    StartNode->CostToStart = 0;
    TArray<FNodeWithPriority> Queue;
    Queue.HeapPush({ StartNode, Heuristic(StartNode) });


    while (!Queue.IsEmpty())
    {
        FNodeWithPriority Current;
        Queue.HeapPop(Current, true);
        UCustomNodeObject* Node = Current.Node;
        if (Node->Visited)
            continue;

        for (const auto& Edge : Node->Edges)
        {
            const FString& Idx = Edge.Idx;
            float Cost = Edge.Distance;
            UCustomNodeObject* Child = Data->ValidNodes.Contains(Idx) ? Data->ValidNodes[Idx] : nullptr;

            if (!Child || Child->Visited)
                continue;

            if (Child->CostToStart < 0 || Node->CostToStart + Cost < Child->CostToStart)
            {
                Child->CostToStart = Node->CostToStart + Cost;
                Child->NearestToStart = Node;
                
            }
        }

        Node->Visited = true;
        NumVisited++;

        if (Node->Idx == TargetNode->Idx)
            return;
    }
}



void AAstarOctree::InsertTempNode(UCustomNodeObject* TempNode)
{
    Data->ValidNodes.Add(TempNode->Idx, TempNode);
    TempNode->Edges.Empty();

    FString CellIdx = PosToIdx(TempNode->Position);
    if (TempNode->Idx == TEXT("Start"))
    {
        StartIdx = CellIdx;
    }
    else if (TempNode->Idx == TEXT("Target"))
    {
        TargetIdx = CellIdx;
    }

    UCustomNodeObject* CellNode = Data->FindNode(CellIdx);
    if (!CellNode) return;

    for (const auto& NeighborGroup : CellNode->Neighbors.Valid)
    {
        for (const FString& NeighborIdx : NeighborGroup.Value.Neighbors)
        {
            FString TransitionIdx;
            if (Data->ValidNodes.Contains(CellNode->Idx + TEXT("&") + NeighborIdx))
            {
                TransitionIdx = CellNode->Idx + TEXT("&") + NeighborIdx;
            }
            else if (Data->ValidNodes.Contains(NeighborIdx + TEXT("&") + CellNode->Idx))
            {
                TransitionIdx = NeighborIdx + TEXT("&") + CellNode->Idx;
            }

            if (!TransitionIdx.IsEmpty())
            {
                float Distance = FVector::Dist(Data->ValidNodes[TransitionIdx]->Position, TempNode->Position);
                Data->ValidNodes[TransitionIdx]->Edges.Add(FEdgeOctree{ TempNode->Idx, Distance });

                if (!TempEdges.Contains(TransitionIdx))
                {
                    TempEdges.Add(TransitionIdx, {});
                }
                TempEdges[TransitionIdx].Add(TPair<FString, float>(TempNode->Idx, Distance));

                TempNode->Edges.Add(FEdgeOctree{ TransitionIdx, Distance });
            }
        }
    }

    StartNode->CostToStart = 0;
    TArray<FNodeWithPriority> Queue;
    Queue.HeapPush({ StartNode, Heuristic(StartNode) });
    // Removed placeholder line inserted during macro replacement

    while (!Queue.IsEmpty())
    {
        FNodeWithPriority Current;
        Queue.HeapPop(Current, true);
        UCustomNodeObject* Node = Current.Node;
        if (Node->Visited)
            continue;

        for (const auto& Edge : Node->Edges)
        {
            const FString& Idx = Edge.Idx;
            float Cost = Edge.Distance;
            UCustomNodeObject* Child = Data->ValidNodes.Contains(Idx) ? Data->ValidNodes[Idx] : nullptr;

            if (!Child || Child->Visited)
                continue;

            if (Child->CostToStart < 0 || Node->CostToStart + Cost < Child->CostToStart)
            {
                Child->CostToStart = Node->CostToStart + Cost;
                Child->NearestToStart = Node;

            }
        }

        Node->Visited = true;
        NumVisited++;

        if (Node->Idx == TargetNode->Idx)
            return;
    }
}

float AAstarOctree::PathLength(const TArray<FVector>& Path)
{
    float TotalLength = 0.0f;
    for (int32 i = 0; i < Path.Num() - 1; ++i)
    {
        TotalLength += FVector::Dist(Path[i], Path[i + 1]);
    }
    return TotalLength;
}

float AAstarOctree::Heuristic(UCustomNodeObject* Node)
{
    if (!Node || !TargetNode)
        return FLT_MAX;

    return FVector::Dist(Node->Position, TargetNode->Position);
}


void AAstarOctree::TestRandomPositions(const FString& Folder) {}
void AAstarOctree::CreateRandomPositions(int32 Count, const FString& Folder) {}

void AAstarOctree::MoveAlongPath() {}

FString AAstarOctree::PosToIdx(const FVector& Position)
{
//    const AOctreeActor* OctreeActor = Cast<AOctreeActor>(GetComponentByClass(AOctreeActor::StaticClass()));
//    if (!OctreeActor)
//        return TEXT("");
//
//    //const int32 GridSize = OctreeActor->GridSize; // adjust to match your octree configuration
//    const FVector Origin = OctreeActor->GetActorLocation();
//    FVector LocalPos = Position - Origin;
//
//    /*int32 X = FMath::FloorToInt(LocalPos.X / GridSize);
//    int32 Y = FMath::FloorToInt(LocalPos.Y / GridSize);
//    int32 Z = FMath::FloorToInt(LocalPos.Z / GridSize);*/
//
//    return FString::Printf(TEXT("%d_%d_%d"), X, Y, Z);
    return FString::Printf(TEXT("Implement"));
}

void AAstarOctree::ComputePathIdx2(const TArray<FVector>& Path)
{
    PathIdx.Empty();
    float TotalDistance = 0.0f;

    for (int32 i = 0; i < Path.Num(); ++i)
    {
        if (i > 0)
        {
            TotalDistance += FVector::Dist(Path[i - 1], Path[i]);
        }
        PathIdx.Add(MakeTuple(Path[i], FString(), TotalDistance));
    }
}


TArray<FVector> AAstarOctree::PrunePath(const TArray<FVector>& Path) { return Path; }

// Additional methods like PosToIdx, InsertTempNode, AStarPath, etc. would follow here
// Each should be carefully translated to handle UE types and logic (e.g., FVector, TArray, UE logging)
// Would you like the next portion of implementation: AStarPath() and AStarSearch() translated next?
