#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "Chunk.generated.h"

/**
 * �`�����N�N���X
 */
UCLASS()
class TERRAINGENERATOR_API AChunk : public ARealtimeMeshActor {
	GENERATED_BODY()
public:
	static constexpr int32 CellCount = 16; // �Z���̗񐔁i���̋����j
	static constexpr float CellScale = 100.0F; // �Z���̑傫��
	static constexpr float UVScale = 1.0F; // UV�̑傫��
	static constexpr float Size = CellCount * CellScale; // ���b�V���̑傫��
	AChunk(); // �R���X�g���N�^
protected:
	virtual void OnGenerateMesh_Implementation() override;
private:
	static constexpr int32 WaterMIdx = 0;
	static constexpr int32 GroundMIdx = 1;
	static inline const TArray<FVector2f> Locations = []() {
		TArray<FVector2f> Locations;
		for(FIntPoint Vert(0); Vert.X <= CellCount; ++Vert.X) for(Vert.Y = 0; Vert.Y <= CellCount; ++Vert.Y) Locations.Add(Vert * CellScale);
		return Locations;
	}();
	static inline const TArray<int32> Triangles = []() {
		TArray<int32> Triangles;
		for(int32 X = 0; X < CellCount; ++X) for(int32 Y = 0; Y < CellCount; ++Y) {
			const int32 VertA = X * (CellCount + 1) + Y, VertB = VertA + 1, VertC = VertB + CellCount, VertD = VertC + 1;
			Triangles.Append((X + Y) % 2 ? TArray<int32>({VertA, VertB, VertC, VertB, VertD, VertC}) : TArray<int32>({VertA, VertB, VertD, VertA, VertD, VertC}));
		}
		return Triangles;
	}();
	static inline const TArray<FVector2f> UVs = []() {
		TArray<FVector2f> UVs;
		for(FIntPoint Vert(0); Vert.X <= CellCount; ++Vert.X) for(Vert.Y = 0; Vert.Y <= CellCount; ++Vert.Y) UVs.Add(Vert * UVScale);
		return UVs;
	}();
}; // �p�b�P�[�W���N���ł��Ȃ��Ȃ�ׁC�w�b�_�[�t�@�C������LoadObject���g�p���Ȃ���