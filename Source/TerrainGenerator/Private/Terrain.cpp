#include "Terrain.h"
#include "Chunk.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

void ATerrain::SetParam(const int32 &TerrainSeed, const int32 &DrawDistance) {
	Seed = TerrainSeed;
	DrawDist = FMath::Square(DrawDistance + 1);
	return;
}

void ATerrain::SetSeed(const int32 &TerrainSeed) {
	Seed = TerrainSeed;
	return;
}

void ATerrain::SetDrawDist(const int32 &DrawDistance) {
	DrawDist = FMath::Square(DrawDistance + 1);
	return;
}

int32 ATerrain::GetSeed() {
	return Seed;
}

int32 ATerrain::GetChunksNum() {
	return GeneratedChunks.Num();
}

/**
 * コンストラクタ
 */
ATerrain::ATerrain() {
	PrimaryActorTick.bCanEverTick = true;
	return;
}

void ATerrain::BeginPlay() {
	Super::BeginPlay();
	GeneratedChunks.Empty();
	return;
}

void ATerrain::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	const FVector PlayerLocation(UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation());
	const FIntPoint Current(FMath::Floor(PlayerLocation.X / AChunk::Size), FMath::Floor(PlayerLocation.Y / AChunk::Size));
	DestroyChunk(Current);
	GenerateChunk(Current);
//	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("Chunk: %d / %d (Num: %d)"), Current.X, Current.Y, Generated.Num()), true, true, FColor::Green, 0.0F, TEXT("None"));
//	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("XYZ: %f / %f / %f"), PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z), true, true, FColor::Green, 0.0F, TEXT("None"));
	return;
}

/**
 * 生成範囲内のチャンクを１つ生成する
 * @param Center 生成範囲中心のチャンク位置
 */
void ATerrain::GenerateChunk(const FIntPoint &Center) const {
	// 生成範囲内で最も近い未生成チャンク位置の取得
	TPair<FIntPoint, int32> Target(FIntPoint::NoneValue, DrawDist); // Key: チャンク位置, Value: チャンク距離
	int32 XDist, Distance;
	for(int32 X = 0; (XDist = X * X) < Target.Value; ++X) for(int32 Y = 0; (Distance = XDist + Y * Y) < Target.Value; ++Y) for(const int32 XSign : {-X, X}) for(const int32 YSign : {-Y, Y}) {
		const FIntPoint Visible(Center.X + XSign, Center.Y + YSign);
		if(!GeneratedChunks.Contains(Visible) && Distance < Target.Value) Target = TPair<FIntPoint, int32>(Visible, Distance);
	}
	// チャンクの生成
	if(Target.Value != DrawDist) GeneratedChunks.Add(Target.Key, GetWorld()->SpawnActor<AChunk>(FVector(Target.Key * AChunk::Size, 0.0), FRotator::ZeroRotator));
	// 終了
	return;
}

/**
 * 生成範囲外のチャンクを１つ削除する
 * @param Center 生成範囲中心のチャンク位置
 */
void ATerrain::DestroyChunk(const FIntPoint &Center) const {
	// 生成範囲外で最も遠い生成済チャンク位置の取得
	TPair<FIntPoint, int32> Target(FIntPoint::NoneValue, 0); // Key: チャンク位置, Value: チャンク距離
	for(const TPair<FIntPoint, AChunk*> &Visible : GeneratedChunks) {
		const int32 Distance = FMath::Square(Visible.Key.X - Center.X) + FMath::Square(Visible.Key.Y - Center.Y);
		if(Distance >= DrawDist && Distance > Target.Value) Target = TPair<FIntPoint, int32>(Visible.Key, Distance);
	}
	// チャンクの削除
	if(Target.Value) GeneratedChunks.FindAndRemoveChecked(Target.Key)->Destroy();
	// 終了
	return;
}