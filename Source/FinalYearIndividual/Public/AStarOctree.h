// AstarOctree.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstarOctree.generated.h"

UCLASS()
class AAstarOctree : public AActor
{
    GENERATED_BODY()

public:
    AAstarOctree();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* StartAndTarget;

private:
    FVector Start;
    FVector Target;

    

    UPROPERTY(EditAnywhere)
    bool bReadFromFile = false;

    UPROPERTY(EditAnywhere)
    bool bDraw = false;

    UPROPERTY(EditAnywhere)
    bool bPrune = false;

    UPROPERTY(EditAnywhere)
    bool bFunnel = false;

    UPROPERTY(EditAnywhere)
    bool bMove = false;

    UPROPERTY(EditAnywhere)
    float CollisionDetectRange = 5.0f;

    class UNodeDataMerged* Data;
    TMap<FString, TArray<TPair<FString, float>>> TempEdges;
    class UCustomNodeObject* StartNode;
    class UCustomNodeObject* TargetNode;

    FString StartIdx;
    FString TargetIdx;

    bool bDone = false;
    int32 NumVisited = 0;
    float DistanceAlongPath = 0.0f;

    TArray<TTuple<FVector, FString, float>> PathIdx;
    AActor* PathDrawer;

    FString PosToIdx(const FVector& Position);
    void InsertTempNode(UCustomNodeObject* TempNode);
    float Heuristic(UCustomNodeObject* Node);
    TTuple<int32, float, double> AStarPath(const FVector& InStart, const FVector& InTarget);
    void AStarSearch();
    TArray<FVector> ComputePath(UCustomNodeObject* S, UCustomNodeObject* T);
    void DrawPath(const TArray<FVector>& Path, FColor Color);
    TArray<FVector> PrunePath(const TArray<FVector>& Path);
    bool IsVisible(const FVector& A, const FVector& B);
    float PathLength(const TArray<FVector>& Path);
    void CreateRandomPositions(int32 Count, const FString& Folder);
    void TestRandomPositions(const FString& Folder);
    TTuple<double, double> CalculateSTD(const TArray<double>& Values);
    void RecomputePath();
    void MoveAlongPath();
    void ComputePathIdx2(const TArray<FVector>& Path);
    FVector ComputeExit(const FVector& U, const FVector& V, UCustomNodeObject* Node);
    TArray<FString> CellsAhead();
};
