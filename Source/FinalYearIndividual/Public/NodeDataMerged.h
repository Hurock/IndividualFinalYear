#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CustomNodeObject.h"
#include "NodeDataMerged.generated.h"

UCLASS()
class UNodeDataMerged : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TMap<FString, UCustomNodeObject*> Cells;

    UPROPERTY()
    TMap<FString, UCustomNodeObject*> ValidNodes;

    UPROPERTY()
    TMap<FString, UCustomNodeObject*> InvalidNodes;

    UPROPERTY()
    TMap<FString, UCustomNodeObject*> GraphNodes;

    UPROPERTY()
    TArray<FString> PathCells;

    UFUNCTION()
    UCustomNodeObject* FindNode(const FString& Idx) const;

    FString GetOppositeDirection(const FString& Direction) const;

    FString IsNeighbor(UCustomNodeObject* A, UCustomNodeObject* B) const;

    TPair<TMap<FString, TArray<FString>>, TMap<FString, TArray<FString>>>
        ComputeChildNeighbors(UCustomNodeObject* Parent, UCustomNodeObject* Child) const;

    void UpdateNeighborsOnSplit(UCustomNodeObject* Parent, const TArray<UCustomNodeObject*>& Children);

    void UpdateNeighborsOnInvalid(UCustomNodeObject* Node);
    void UpdateNeighborsOnValid(UCustomNodeObject* Node);
    void UpdateNeighborsOnMerge(UCustomNodeObject* Parent);
};