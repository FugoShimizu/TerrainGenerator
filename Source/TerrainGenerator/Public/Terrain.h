#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain.generated.h"

class AChunk;

/**
 * 地形クラス
 */
UCLASS()
class TERRAINGENERATOR_API ATerrain : public AActor {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	static void SetParam(const int32 &TerrainSeed, const int32 &DrawDistance);
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	static void SetSeed(const int32 &TerrainSeed);
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	static void SetDrawDist(const int32 &DrawDistance);
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	static int32 GetSeed();
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	static int32 GetChunksNum();
	ATerrain(); // コンストラクタ
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
private:
	static inline int32 Seed = 0; // シード値
	static inline int32 DrawDist = 625; // 描画距離（チャンク数）の二乗
	static inline TMap<FIntPoint, AChunk*> GeneratedChunks; // 生成済チャンク集合
	void GenerateChunk(const FIntPoint &Center) const; // チャンク生成関数
	void DestroyChunk(const FIntPoint &Center) const; // チャンク削除関数
};