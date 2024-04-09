#include "Chunk.h"
#include "Terrain.h"
#include "NoiseGenerator.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshAlgo.h"

/**
 * �R���X�g���N�^
 */
AChunk::AChunk() {
	PrimaryActorTick.bCanEverTick = false;
	return;
}

/**
 * ���b�V���𐶐�����	
 */
void AChunk::OnGenerateMesh_Implementation() {
	Super::OnGenerateMesh_Implementation();
	// �P�����b�V���̏�����
	URealtimeMeshSimple *RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	// �X�g���[���f�[�^�̐ݒ�
	FRealtimeMeshStreamSet WaterStream, GroundStream;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2f, 1> BuilderW(WaterStream), BuilderG(GroundStream);
	BuilderW.EnableTangents();
	BuilderW.EnableTexCoords();
	BuilderW.EnablePolyGroups();
	BuilderW.EnableColors(); // �폜����ƃN���b�V������
	BuilderG.EnableTangents();
	BuilderG.EnableTexCoords();
	BuilderG.EnablePolyGroups();
	BuilderG.EnableColors(); // �폜����ƃN���b�V������
	// �f�[�^�p�z��
	TArray<FVector3f> VerticesW, VerticesG, NormalsW, NormalsG, TangentsW, TangentsG;
	// �W���̎Z�o
	const FVector2f Origin = FVector2f(FVector3f(GetActorLocation()));
	NoiseGenerator Noise(ATerrain::GetSeed());
	for(const FVector2f &Location : Locations) {
		const FVector2f Elevation = Noise.CalculateElevation(Location + Origin);
		VerticesW.Add(FVector3f(Location, Elevation.X));
		VerticesG.Add(FVector3f(Location, Elevation.Y));
		NormalsW.Add(FVector3f(0.0F, 0.0F, 1.0F));
		NormalsG.Add(FVector3f(0.0F, 0.0F, 1.0F));
		TangentsW.Add(FVector3f(0.0F, -1.0F, 0.0F));
		TangentsG.Add(FVector3f(0.0F, -1.0F, 0.0F));
	}
	// �ڐ��Ɩ@���̎Z�o
	RealtimeMeshAlgo::GenerateTangents(
		TConstArrayView<int32>(Triangles), VerticesW,
		[](int Index) {return UVs[Index]; },
		[&NormalsW, &TangentsW](int Index, FVector3f Tangent, FVector3f Normal) {
			NormalsW[Index] = Normal;
			TangentsW[Index] = Tangent;
		},
		true
	);
	RealtimeMeshAlgo::GenerateTangents(
		TConstArrayView<int32>(Triangles), VerticesG,
		[](int Index) {return UVs[Index]; },
		[&NormalsG, &TangentsG](int Index, FVector3f Tangent, FVector3f Normal) {
			NormalsG[Index] = Normal;
			TangentsG[Index] = Tangent;
		},
		true
	);
	// ���b�V���̍\�z
	for(int32 Index = 0; Index < Locations.Num(); ++Index) {
		BuilderW.AddVertex(VerticesW[Index]).SetNormalAndTangent(NormalsW[Index], TangentsW[Index]).SetTexCoord(UVs[Index]);
		BuilderG.AddVertex(VerticesG[Index]).SetNormalAndTangent(NormalsG[Index], TangentsG[Index]).SetTexCoord(UVs[Index]);
	}
	// �O�p�`�̍\�z
	for(int32 Index = 0; Index < Triangles.Num(); Index += 3) {
		BuilderW.AddTriangle(Triangles[Index], Triangles[Index + 1], Triangles[Index + 2], WaterMIdx);
		BuilderG.AddTriangle(Triangles[Index], Triangles[Index + 1], Triangles[Index + 2], GroundMIdx);
	}
	// �}�e���A���̐ݒ�
	RealtimeMesh->SetupMaterialSlot(WaterMIdx, "PrimaryMaterial", LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FirstPerson/Materials/Water_FarMesh")));
	RealtimeMesh->SetupMaterialSlot(GroundMIdx, "SecondaryMaterial", LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Ground_Gravel")));
	// �Z�N�V�����̐ݒ�
	const FRealtimeMeshSectionGroupKey WaterGroupKey(FRealtimeMeshSectionGroupKey::Create(0, "Water")), GroundGroupKey(FRealtimeMeshSectionGroupKey::Create(0, "Ground"));
	RealtimeMesh->CreateSectionGroup(WaterGroupKey, WaterStream);
	RealtimeMesh->CreateSectionGroup(GroundGroupKey, GroundStream);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(WaterGroupKey, WaterMIdx), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, WaterMIdx), false);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(GroundGroupKey, GroundMIdx), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, GroundMIdx), true);
	// �I��
	return;
}