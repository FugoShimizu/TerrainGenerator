#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Minimap.generated.h"

UCLASS()
class TERRAINGENERATOR_API UMinimap : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static TArray<FVector2f> GetPointLocations(const FVector2f &CenterLocation, const int32 &PointCount, const float &PointDist);
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static void UpdateRowColors(const TArray<FVector2f> &PointLocations, UPARAM(ref) TArray<FLinearColor> &PixelColors, const int32 &PixelCount, int32 &RowNum, const float &MaxElevation, const bool &DrawRiver = false);
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static void DrawMinimap(UPARAM(ref) FPaintContext &Context, const TArray<FLinearColor> &PixelColors, const int32 &PixelCount, const double &ViewDirection, const FVector2D &Offset, const double &Size, USlateBrushAsset *Brush);
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	static void DrawGrid(UPARAM(ref) FPaintContext &Context, const double &ViewDirection, const FVector2D &Offset, const double &Size);
};