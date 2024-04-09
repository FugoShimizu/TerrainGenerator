#include "Minimap.h"
#include "Terrain.h"
#include "NoiseGenerator.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

TArray<FVector2f> UMinimap::GetPointLocations(const FVector2f &CenterLocation, const int32 &PointCount, const float &PointDist) {
	const FVector2f Offset = CenterLocation - (PointCount - 1) * PointDist * 0.5F;
	TArray<FVector2f> Locations;
	for(int32 X = 0; X < PointCount; ++X) for(int32 Y = 0; Y < PointCount; ++Y) Locations.Add(Offset + FVector2f(X, Y) * PointDist);
	return Locations;
}

void UMinimap::UpdateRowColors(const TArray<FVector2f> &PointLocations, TArray<FLinearColor> &PixelColors, const int32 &PixelCount, int32 &RowNum, const float &MaxElevation, const bool &DrawRiver) {
	const int32 PixelCountSqr = PixelCount * PixelCount;
	if(PointLocations.Num() < PixelCountSqr) return;
	if(PixelColors.Num() < PixelCountSqr) PixelColors.Init(FLinearColor::Green, PixelCountSqr);
	NoiseGenerator Noise(ATerrain::GetSeed());
	for(int32 Index = PixelCount * RowNum; Index < PixelCount * (RowNum + 1); ++Index) {
		const FVector2f Elevation = Noise.CalculateElevation(PointLocations[Index]);
		const float Scale = Elevation.Y * 2.0F / MaxElevation;
		PixelColors[Index] = DrawRiver && Scale > 0.0F && Elevation.X > Elevation.Y ? FLinearColor(0.0F, 1.0F, 1.0F) : FLinearColor(FMath::Abs(Scale + 0.5F) - 0.5F, 1.5F - FMath::Abs(Scale - 0.5F), Scale < 0.0F);
	}
	++RowNum %= PixelCount;
	return;
}

void UMinimap::DrawMinimap(FPaintContext &Context, const TArray<FLinearColor> &PixelColors, const int32 &PixelCount, const double &ViewDirection, const FVector2D &Offset, const double &Size, USlateBrushAsset *Brush) {
	if(PixelColors.Num() < PixelCount * PixelCount) return;
	const double PixelSize = Size / PixelCount;
	int32 Index = 0;
	for(int32 Y = PixelCount - 1; Y >= 0; --Y) for(int32 X = 0; X < PixelCount; ++X) UWidgetBlueprintLibrary::DrawBox(Context, Offset + FVector2D(X, Y) * PixelSize, FVector2D(PixelSize), Brush, PixelColors[Index++]);
	DrawGrid(Context, ViewDirection, Offset, Size);
	return;
}

void UMinimap::DrawGrid(FPaintContext &Context, const double &ViewDirection, const FVector2D &Offset, const double &Size) {
	const double HalfSize = Size * 0.5;
	UWidgetBlueprintLibrary::DrawLine(Context, Offset, Offset + FVector2D(0.0, Size));
	UWidgetBlueprintLibrary::DrawLine(Context, Offset + FVector2D(HalfSize, 0.0), Offset + FVector2D(HalfSize, Size));
	UWidgetBlueprintLibrary::DrawLine(Context, Offset + FVector2D(Size, 0.0), Offset + Size);
	UWidgetBlueprintLibrary::DrawLine(Context, Offset, Offset + FVector2D(Size, 0.0));
	UWidgetBlueprintLibrary::DrawLine(Context, Offset + FVector2D(0.0, HalfSize), Offset + FVector2D(Size, HalfSize));
	UWidgetBlueprintLibrary::DrawLine(Context, Offset + FVector2D(0.0, Size), Offset + Size);
	UWidgetBlueprintLibrary::DrawLine(Context, Offset + HalfSize, Offset + HalfSize + FVector2D(0.0, HalfSize * -0.5).GetRotated(ViewDirection));
	return;
}