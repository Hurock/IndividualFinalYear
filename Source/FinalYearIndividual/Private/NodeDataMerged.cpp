#include "NodeDataMerged.h"

UCustomNodeObject* UNodeDataMerged::FindNode(const FString& Idx) const
{
    if (Cells.Contains(Idx)) return Cells[Idx];
    if (ValidNodes.Contains(Idx)) return ValidNodes[Idx];
    if (InvalidNodes.Contains(Idx)) return InvalidNodes[Idx];
    return nullptr;
}

FString UNodeDataMerged::GetOppositeDirection(const FString& Direction) const
{
    if (Direction == "up") return "down";
    if (Direction == "down") return "up";
    if (Direction == "left") return "right";
    if (Direction == "right") return "left";
    if (Direction == "forward") return "backward";
    if (Direction == "backward") return "forward";
    return "";
}

FString UNodeDataMerged::IsNeighbor(UCustomNodeObject* A, UCustomNodeObject* B) const
{
    if (!A || !B) return "";

    auto Overlaps = [](float min1, float max1, float min2, float max2)
        {
            return max1 > min2 && min1 < max2;
        };

    FVector AP = A->Position, AS = A->Scale;
    FVector BP = B->Position, BS = B->Scale;

    // Check X-aligned neighbors
    if ((AP.X + AS.X / 2 == BP.X - BS.X / 2 || AP.X - AS.X / 2 == BP.X + BS.X / 2) &&
        Overlaps(AP.Y - AS.Y / 2, AP.Y + AS.Y / 2, BP.Y - BS.Y / 2, BP.Y + BS.Y / 2) &&
        Overlaps(AP.Z - AS.Z / 2, AP.Z + AS.Z / 2, BP.Z - BS.Z / 2, BP.Z + BS.Z / 2))
        return (AP.X < BP.X) ? "right" : "left";

    // Check Y-aligned neighbors
    if ((AP.Y + AS.Y / 2 == BP.Y - BS.Y / 2 || AP.Y - AS.Y / 2 == BP.Y + BS.Y / 2) &&
        Overlaps(AP.X - AS.X / 2, AP.X + AS.X / 2, BP.X - BS.X / 2, BP.X + BS.X / 2) &&
        Overlaps(AP.Z - AS.Z / 2, AP.Z + AS.Z / 2, BP.Z - BS.Z / 2, BP.Z + BS.Z / 2))
        return (AP.Y < BP.Y) ? "up" : "down";

    // Check Z-aligned neighbors
    if ((AP.Z + AS.Z / 2 == BP.Z - BS.Z / 2 || AP.Z - AS.Z / 2 == BP.Z + BS.Z / 2) &&
        Overlaps(AP.X - AS.X / 2, AP.X + AS.X / 2, BP.X - BS.X / 2, BP.X + BS.X / 2) &&
        Overlaps(AP.Y - AS.Y / 2, AP.Y + AS.Y / 2, BP.Y - BS.Y / 2, BP.Y + BS.Y / 2))
        return (AP.Z < BP.Z) ? "forward" : "backward";

    return "";
}

TPair<TMap<FString, TArray<FString>>, TMap<FString, TArray<FString>>>
UNodeDataMerged::ComputeChildNeighbors(UCustomNodeObject* Parent, UCustomNodeObject* Child) const
{
    TMap<FString, TArray<FString>> NewValid;
    for (const auto& Pair : Parent->Neighbors.Valid)
    {
        NewValid.Add(Pair.Key, Pair.Value.Neighbors);
    }

    TMap<FString, TArray<FString>> NewInvalid;
    for (const auto& Pair : Parent->Neighbors.Invalid)
    {
        NewInvalid.Add(Pair.Key, Pair.Value.Neighbors);
    }

    for (const FString& Direction : { "up", "down", "left", "right", "forward", "backward" })
    {
        TArray<FString>& Valid = NewValid[Direction];
        Valid.RemoveAll([&](const FString& Idx) {
            return IsNeighbor(FindNode(Idx), Child).IsEmpty();
            });

        TArray<FString>& Invalid = NewInvalid[Direction];
        Invalid.RemoveAll([&](const FString& Idx) {
            return IsNeighbor(FindNode(Idx), Child).IsEmpty();
            });
    }

    return TPair<TMap<FString, TArray<FString>>, TMap<FString, TArray<FString>>>(NewValid, NewInvalid);
}

void UNodeDataMerged::UpdateNeighborsOnSplit(UCustomNodeObject* Parent, const TArray<UCustomNodeObject*>& Children)
{
    auto Update = [&](const TMap<FString, FNeighborList>& Map, bool bIsValid)
        {
            for (const auto& Pair : Map)
            {
                const FString& Direction = Pair.Key;
                for (const FString& NeighborId : Pair.Value.Neighbors)
                {
                    UCustomNodeObject* Neighbor = FindNode(NeighborId);
                    if (!Neighbor) continue;

                    TArray<FString>* TargetList = nullptr;
                    FString Opposite = GetOppositeDirection(Direction);

                    if (bIsValid)
                    {
                        TargetList = &Neighbor->Neighbors.Valid[Opposite].Neighbors;
                    }
                    else
                    {
                        TargetList = &Neighbor->Neighbors.Invalid[Opposite].Neighbors;
                    }

                    if (TargetList)
                    {
                        TargetList->Remove(Parent->Idx);

                        for (UCustomNodeObject* Child : Children)
                        {
                            if (!IsNeighbor(Neighbor, Child).IsEmpty())
                            {
                                TargetList->AddUnique(Child->Idx);
                            }
                        }
                    }

                    TargetList->Remove(Parent->Idx);

                    for (UCustomNodeObject* Child : Children)
                    {
                        if (!IsNeighbor(Neighbor, Child).IsEmpty())
                            TargetList->AddUnique(Child->Idx);
                    }
                }
            }
        };

    Update(Parent->Neighbors.Valid, true);
    Update(Parent->Neighbors.Invalid, false);
}

void UNodeDataMerged::UpdateNeighborsOnInvalid(UCustomNodeObject* Node)
{
    for (const auto& Pair : Node->Neighbors.Valid)
    {
        const FString& Dir = Pair.Key;
        for (const FString& NeighborId : Pair.Value.Neighbors)
        {
            UCustomNodeObject* Neighbor = FindNode(NeighborId);
            if (Neighbor)
            {
                Neighbor->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Remove(Node->Idx);
                Neighbor->Neighbors.Invalid[GetOppositeDirection(Dir)].Neighbors.Add(Node->Idx);
            }
        }
    }

    for (const auto& Pair : Node->Neighbors.Invalid)
    {
        const FString& Dir = Pair.Key;
        for (const FString& NeighborId : Pair.Value.Neighbors)
        {
            UCustomNodeObject* Neighbor = FindNode(NeighborId);
            if (Neighbor)
            {
                Neighbor->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Remove(Node->Idx);
                Neighbor->Neighbors.Invalid[GetOppositeDirection(Dir)].Neighbors.Add(Node->Idx);
            }
        }
    }
}

void UNodeDataMerged::UpdateNeighborsOnValid(UCustomNodeObject* Node)
{
    for (const auto& Pair : Node->Neighbors.Valid)
    {
        const FString& Dir = Pair.Key;
        for (const FString& NeighborId : Pair.Value.Neighbors)
        {
            UCustomNodeObject* Neighbor = FindNode(NeighborId);
            if (Neighbor)
            {
                Neighbor->Neighbors.Invalid[GetOppositeDirection(Dir)].Neighbors.Remove(Node->Idx);
                Neighbor->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Add(Node->Idx);
            }
        }
    }

    for (const auto& Pair : Node->Neighbors.Invalid)
    {
        const FString& Dir = Pair.Key;
        for (const FString& NeighborId : Pair.Value.Neighbors)
        {
            UCustomNodeObject* Neighbor = FindNode(NeighborId);
            if (Neighbor)
            {
                Neighbor->Neighbors.Invalid[GetOppositeDirection(Dir)].Neighbors.Remove(Node->Idx);
                Neighbor->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Add(Node->Idx);
            }
        }
    }
}

void UNodeDataMerged::UpdateNeighborsOnMerge(UCustomNodeObject* Parent)
{
    const TArray<FString> Directions = { "up", "down", "left", "right", "forward", "backward" };

    for (const FString& Dir : Directions)
    {
        Parent->Neighbors.Valid[Dir].Neighbors.Empty();
        Parent->Neighbors.Invalid[Dir].Neighbors.Empty();
    }

    for (const FString& ChildIdx : Parent->Children)
    {
        UCustomNodeObject* Child = FindNode(ChildIdx);
        if (!Child) continue;

        for (const FString& Dir : Directions)
        {
            for (const FString& NeighIdx : Child->Neighbors.Valid[Dir].Neighbors)
            {
                if (FindNode(NeighIdx)->Parent == Child->Parent) continue;
                Parent->Neighbors.Valid[Dir].Neighbors.AddUnique(NeighIdx);
                FindNode(NeighIdx)->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Remove(Child->Idx);
            }

            for (const FString& NeighIdx : Child->Neighbors.Invalid[Dir].Neighbors)
            {
                Parent->Neighbors.Invalid[Dir].Neighbors.AddUnique(NeighIdx);
                FindNode(NeighIdx)->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.Remove(Child->Idx);
            }
        }
    }

    for (const FString& Dir : Directions)
    {
        for (const FString& NeighIdx : Parent->Neighbors.Valid[Dir].Neighbors)
            FindNode(NeighIdx)->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.AddUnique(Parent->Idx);

        for (const FString& NeighIdx : Parent->Neighbors.Invalid[Dir].Neighbors)
            FindNode(NeighIdx)->Neighbors.Valid[GetOppositeDirection(Dir)].Neighbors.AddUnique(Parent->Idx);
    }
}