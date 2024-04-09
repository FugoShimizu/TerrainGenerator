#include "Chunk.h"
#include "Terrain.h"
#include "NoiseGenerator.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshAlgo.h"

/**
 * コンストラクタ
 */
AChunk::AChunk() {
	PrimaryActorTick.bCanEverTick = false;
	return;
}

/**
 * メッシュを生成する	
 */
void AChunk::OnGenerateMesh_Implementation() {
	Super::OnGenerateMesh_Implementation();
	// 単純メッシュの初期化
	URealtimeMeshSimple *RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	// ストリームデータの設定
	FRealtimeMeshStreamSet WaterStream, GroundStream;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2f, 1> BuilderW(WaterStream), BuilderG(GroundStream);
	BuilderW.EnableTangents();
	BuilderW.EnableTexCoords();
	BuilderW.EnablePolyGroups();
	BuilderW.EnableColors(); // 削除するとクラッシュする
	BuilderG.EnableTangents();
	BuilderG.EnableTexCoords();
	BuilderG.EnablePolyGroups();
	BuilderG.EnableColors(); // 削除するとクラッシュする
	// データ用配列
	TArray<FVector3f> VerticesW, VerticesG, NormalsW, NormalsG, TangentsW, TangentsG;
	// 標高の算出
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
	// 接線と法線の算出
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
	// メッシュの構築
	for(int32 Index = 0; Index < Locations.Num(); ++Index) {
		BuilderW.AddVertex(VerticesW[Index]).SetNormalAndTangent(NormalsW[Index], TangentsW[Index]).SetTexCoord(UVs[Index]);
		BuilderG.AddVertex(VerticesG[Index]).SetNormalAndTangent(NormalsG[Index], TangentsG[Index]).SetTexCoord(UVs[Index]);
	}
	// 三角形の構築
	for(int32 Index = 0; Index < Triangles.Num(); Index += 3) {
		BuilderW.AddTriangle(Triangles[Index], Triangles[Index + 1], Triangles[Index + 2], WaterMIdx);
		BuilderG.AddTriangle(Triangles[Index], Triangles[Index + 1], Triangles[Index + 2], GroundMIdx);
	}
	// マテリアルの設定
	RealtimeMesh->SetupMaterialSlot(WaterMIdx, "PrimaryMaterial", LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FirstPerson/Materials/Water_FarMesh")));
	RealtimeMesh->SetupMaterialSlot(GroundMIdx, "SecondaryMaterial", LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/StarterContent/Materials/M_Ground_Gravel")));
	// セクションの設定
	const FRealtimeMeshSectionGroupKey WaterGroupKey(FRealtimeMeshSectionGroupKey::Create(0, "Water")), GroundGroupKey(FRealtimeMeshSectionGroupKey::Create(0, "Ground"));
	RealtimeMesh->CreateSectionGroup(WaterGroupKey, WaterStream);
	RealtimeMesh->CreateSectionGroup(GroundGroupKey, GroundStream);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(WaterGroupKey, WaterMIdx), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, WaterMIdx), false);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(GroundGroupKey, GroundMIdx), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, GroundMIdx), true);
	// 終了
	return;
}