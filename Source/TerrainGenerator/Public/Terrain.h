#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain.generated.h"

class AChunk;

/**
 * �n�`�N���X
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
	ATerrain(); // �R���X�g���N�^
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
private:
	static inline int32 Seed = 0; // �V�[�h�l
	static inline int32 DrawDist = 625; // �`�拗���i�`�����N���j�̓��
	static inline TMap<FIntPoint, AChunk*> GeneratedChunks; // �����σ`�����N�W��
	void GenerateChunk(const FIntPoint &Center) const; // �`�����N�����֐�
	void DestroyChunk(const FIntPoint &Center) const; // �`�����N�폜�֐�
};