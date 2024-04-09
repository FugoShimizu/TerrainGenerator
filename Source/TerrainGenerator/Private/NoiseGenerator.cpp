#include "NoiseGenerator.h"

/**
 * コンストラクタ
 * @param Seed シード値
 */
NoiseGenerator::NoiseGenerator(const int32 &Seed) {
	NoiseSeed = Seed;
	SimplexNoise = UFastNoise2BlueprintLibrary::MakeSimplexGenerator();
	return;
}

/**
 * 標高を算出する
 * @param Location 当該地点の平面座標
 * @return 当該地点の標高
 */
FVector2f NoiseGenerator::CalculateElevation(const FVector2f &Location) {
	// 標高の算出
	const float LandNoise = FMath::Cube(GetSimplexNoise(Location, 1.0F));
	const float MainNoise = FMath::Square(GetSimplexNoise(Location));
	const float MiniNoise = FMath::Square(GetSimplexNoise(Location * 128.0F, 64.0F));
	float Elevation = (LandNoise + MainNoise * SwellScale) * SimpScaleV; // 標高
	float WaterLevel = 0.0F;
	// 水域の生成
	if(Elevation > 0.0F) { // 大陸
		// 当該地点のセルの位置の取得
		const FVector2f ScaleLoc(Location / CellScaleH);
		const int32 OriginX = FMath::Floor(ScaleLoc.X) - 2, OriginY = FMath::Floor(ScaleLoc.Y) - 2;
		// 探索範囲内の頂点の取得
		TSet<FIntPoint> TargetVertices;
		for(int32 X = OriginX; X < OriginX + 5; ++X) for(int32 Y = OriginY; Y < OriginY + 5; ++Y) TargetVertices.Add({X, Y});
		// 各対象河川の探索範囲内の最上流の頂点の取得
		TSet<FIntPoint> SourceVertices(TargetVertices);
		for(int32 X = OriginX; X < OriginX + 5; ++X) for(int32 Y = OriginY; Y < OriginY + 5; ++Y) SourceVertices.Remove(GetVerConnection({X, Y}));
		// 山脈の造成
		const FVector2f PathDist(GetDistSquaredToPath(ScaleLoc, TargetVertices)); // １番目と２番目に近いパス迄の二乗距離
		const float Add = PathDist.X + PathDist.Y;
		const float Sub = PathDist.Y - PathDist.X;
		const float Rnd = LandNoise * RidgeRound + 1.0F;
		WaterLevel = Elevation += (Add < Sub * Rnd ? PathDist.X : (Add * 2.0F - Sub * Sub * Rnd / Add - Add / Rnd) * 0.25F) * (LandNoise + MainNoise * SwellScale) * MainNoise * CellScaleV;
		// 河川の造成
		const FVector2f RiverDist(GetDistSquaredToRiver(ScaleLoc, SourceVertices, TargetVertices)); // １番目と２番目に近い河川迄の二乗距離
		const float CorrectedElev = Elevation * RiverWidth;
		if(RiverDist.X * CorrectedElev < 1.0F) { // 河川 https://www.desmos.com/calculator/jzjsxtodsn
			const float RivSpacing = RiverDist.Y != FLT_MAX ? FastSqrt(RiverDist.X * RiverDist.Y) * 0.5F + (RiverDist.X + RiverDist.Y) * 0.25F : FLT_MAX;
			const float Riverbed = RivSpacing * CorrectedElev < 1.0F ? (FMath::Square(1.0F - RiverDist.X / RivSpacing) - 1.0F) * RivSpacing * CorrectedElev + 1.0F : FMath::Square(1.0F - RiverDist.X * CorrectedElev);
			const float ClimbRate = Elevation * 0.875F / (Elevation + RiverSlope);
			Elevation -= (1.0F - ClimbRate) * Riverbed * WaterDepth - (1.0F - Riverbed * ClimbRate) * (MainNoise + 1.0F) * MiniNoise * StoneScale;
		} else Elevation += (MainNoise + 1.0F) * MiniNoise * StoneScale; // 陸上
	//	if(RiverDist.X < 2.44140625E-4F) Elevation = 0.0F;
	} else { // 海洋
		Elevation -= WaterDepth - (MainNoise + 1.0F) * MiniNoise * StoneScale;
	}
	// 終了
	return FVector2f(WaterLevel, Elevation);
}

/**
 * 頂点の接続先を取得する
 * @param Position 頂点の位置
 * @return 頂点の接続先
 */
FIntPoint NoiseGenerator::GetVerConnection(const FIntPoint &Position) {
	// マップに頂点の接続先が有れば返戻
	const FIntPoint *ConnectionPtr(VerConnection.Find(Position));
	if(ConnectionPtr) return *ConnectionPtr;
	// マップに無ければ導出
	const FVector3f Location(GetVerLocation(Position));
	TPair<FIntPoint, float> Connection(Position, FLT_MAX);
	for(int32 X = -1; X <= 1; ++X) for(int32 Y = -1; Y <= 1; ++Y) if(X || Y) {
		const FIntPoint AdjPosition(Position.X + X, Position.Y + Y);
		const FVector3f AdjLocation(GetVerLocation(AdjPosition));
		if(AdjLocation.Z < Location.Z) {
			const float Distance = FVector3f::DistSquared2D(Location, AdjLocation);
			if(Distance < Connection.Value) Connection = {AdjPosition, Distance};
		}
	}
	// マップに頂点の接続先を追加して返戻
	return VerConnection.Add(Position, Connection.Key);
}

/**
 * 頂点の座標を取得する
 * @param Position 頂点の位置
 * @return 頂点の座標
 */
FVector3f NoiseGenerator::GetVerLocation(const FIntPoint &Position) {
	// マップに頂点の座標が有れば返戻
	const FVector3f *LocationPtr(VerLocation.Find(Position));
	if(LocationPtr) return *LocationPtr;
	// マップに無ければ導出
	const int32 Idx = Hash(Position);
	const FVector2f Location2D(Position.X + RandVecs[Idx], Position.Y + RandVecs[Idx | 1]);
	//	const FVector2f Location2D(Position.X + RandVecs[Idx] * 0.75F, Position.Y + RandVecs[Idx | 1] * 0.75F);
	const FVector3f Location(Location2D, GetSimplexNoise(Location2D * CellScaleH, 1.0F));
	// マップに頂点の座標を追加して返戻
	return VerLocation.Add(Position, Location);
}

/**
 * ハッシュ値を生成する
 * @param Position セルの位置
 * @return ハッシュ値
 */
int32 NoiseGenerator::Hash(const FIntPoint &Position) const {
	const int32 PrimeX = 501125321, PrimeY = 1136930381;
	int32 hash = NoiseSeed ^ Position.X * PrimeX ^ Position.Y * PrimeY;
	hash *= 0x27d4eb2d;
	return hash & (255 << 1);
}

/**
 * シンプレックスノイズの値を取得する
 * @param Location ノイズ取得座標
 * @param MaxScale
 * @return 標高
 */
float NoiseGenerator::GetSimplexNoise(const FVector2f &Location, const float &MaxScale) {
	float Noise = 0.0F;
	const FVector2D ScaleLoc(Location / SimpScaleH);
	for(float Scale = 1.0F; Scale <= MaxScale; Scale *= 2.0F) Noise += SimplexNoise->GenSingle2D(ScaleLoc * Scale, NoiseSeed) / Scale;
	return Noise;
}

/**
 * １番目と２番目に近いパス迄の二乗距離を取得する
 * @param Point 点の座標
 * @param TargetVertices 探索範囲内の頂点
 * @return １番目と２番目に近いパス迄の距離（X: １番目, Y: ２番目）
 */
FVector2f NoiseGenerator::GetDistSquaredToPath(const FVector2f &Point, const TSet<FIntPoint> &TargetVertices) {
	// ベクトルを格納するマップ
	TMap<FIntPoint, TPair<FVector2f, float>> Vectors;
	// パス線迄の距離を取得してマップに追加して近い順にソート
	for(const FIntPoint &Vertex : TargetVertices) {
		const FVector2f Vector(GetVecToLineSegment(Point, FVector2f(GetVerLocation(Vertex)), FVector2f(GetVerLocation(GetVerConnection(Vertex)))));
		Vectors.Add(Vertex, {Vector, Vector.SizeSquared()});
	}
	Vectors.ValueSort([](const TPair<FVector2f, float> &A, const TPair<FVector2f, float> &B) {return A.Value < B.Value; });
	// １番目と２番目に近いパス迄の距離の取得
	bool First = true;
	FVector2f MinVector, Distances(FLT_MAX);
	for(const TPair<FIntPoint, TPair<FVector2f, float>> &Vector : Vectors) if(First) {
		First = false;
		MinVector = Vector.Value.Key;
		Distances.X = Vector.Value.Value; // １番目に近いパス迄の距離
	} else if(FVector2f::DotProduct(MPoint(FVector2f(GetVerLocation(Vector.Key)), FVector2f(GetVerLocation(GetVerConnection(Vector.Key)))) - Point, MinVector) < Distances.X) {
		Distances.Y = Vector.Value.Value; // ２番目に近いパス迄の距離
		return Distances;
	}
	// １番目と２番目に近いパス迄の距離の返戻
	return Distances;
}

/**
 * １番目と２番目に近い河川迄の二乗距離を取得する
 * @param Point 点の座標
 * @param SourceVertices 各河川の探索範囲内の最上流の頂点
 * @param TargetVertices 探索範囲内の頂点
 * @return １番目と２番目に近い河川迄の距離（X: １番目, Y: ２番目）
 */
FVector2f NoiseGenerator::GetDistSquaredToRiver(const FVector2f &Point, const TSet<FIntPoint> &SourceVertices, const TSet<FIntPoint> &TargetVertices) {
	// ベクトルを格納するマップ
	TMap<FIntPoint, TPair<FVector2f, float>> Vectors, MinVectors;
	// 河川中心線迄の距離を取得してマップに追加して近い順にソート
	for(const FIntPoint &Vertex : SourceVertices) MinVectors.Add(Vertex, GetVecToRiver(Point, Vertex, TargetVertices, Vectors));
	MinVectors.ValueSort([](const TPair<FVector2f, float> &A, const TPair<FVector2f, float> &B) {return A.Value < B.Value; });
	// １番目と２番目に近い河川迄の距離の取得
	bool First = true;
	FIntPoint SourceVertex;
	FVector2f MinVector, Distances(FLT_MAX);
	for(const TPair<FIntPoint, TPair<FVector2f, float>> &Vector : MinVectors) if(First) {
		First = false;
		SourceVertex = Vector.Key;
		MinVector = Vector.Value.Key;
		Distances.X = Vector.Value.Value; // １番目に近い河川迄の距離
	} else if(Vector.Value.Key != MinVector && !IsOverRiver(Point, Vector.Value.Key, SourceVertex, TargetVertices)) {
		Distances.Y = Vector.Value.Value; // ２番目に近い河川迄の距離
		return Distances;
	}
	// １番目と２番目に近い河川迄の距離の返戻
	return Distances;
}

/**
 * 点から河川中心線迄の最短ベクトルを取得する
 * @param Point 点の座標
 * @param SourceVertex 対象河川の探索範囲内の最上流の頂点
 * @param TargetVertices 探索範囲内の頂点
 * @param Vectors 河川中心線の各区間迄の最短ベクトルを格納するマップ
 * @return 点から河川中心線の最短ベクトル
 */
TPair<FVector2f, float> NoiseGenerator::GetVecToRiver(const FVector2f &Point, const FIntPoint &SourceVertex, const TSet<FIntPoint> &TargetVertices, TMap<FIntPoint, TPair<FVector2f, float>> &Vectors) {
	// マップに河川中心線迄のベクトルが有れば返戻
	const TPair<FVector2f, float> *VectorPtr(Vectors.Find(SourceVertex));
	if(VectorPtr) return *VectorPtr;
	// 次の頂点を取得して範囲外なら終了
	const FIntPoint NextVertex(GetVerConnection(SourceVertex));
	if(NextVertex == SourceVertex || !TargetVertices.Contains(NextVertex)) return Vectors.Add(SourceVertex, {FVector2f::ZeroVector, FLT_MAX});
	// 次の次の頂点を取得して範囲外なら終了
	const FIntPoint AfterNextVertex(GetVerConnection(NextVertex));
	if(AfterNextVertex == NextVertex || !TargetVertices.Contains(AfterNextVertex)) return Vectors.Add(SourceVertex, {FVector2f::ZeroVector, FLT_MAX});
	// 各基準点の設定
	const FVector2f ControlPoint(GetVerLocation(NextVertex));
	const FVector2f StartPoint(MPoint(FVector2f(GetVerLocation(SourceVertex)), ControlPoint));
	const FVector2f EndPoint(MPoint(FVector2f(GetVerLocation(AfterNextVertex)), ControlPoint));
	// 河川中心線迄のベクトルを導出
	const FVector2f Vector(GetVecToBezierCurve(Point, StartPoint, ControlPoint, EndPoint));
	const float Distance = Vector.SizeSquared();
	// マップに現在の頂点から範囲内の最下流迄の最小値を追加して返戻
	const TPair<FVector2f, float> Downstream(GetVecToRiver(Point, NextVertex, TargetVertices, Vectors));
	return Vectors.Add(SourceVertex, Distance < Downstream.Value ? TPair<FVector2f, float>(Vector, Distance) : Downstream);
}

/**
 * ベクトルの河川中心線との交差を判定する
 * @param Point ベクトルの始点座標
 * @param Vector ベクトル
 * @param SourceVertex 対象河川の探索範囲内の最上流の頂点
 * @param TargetVertices 探索範囲内の頂点
 * @return true: 交差有り, false: 交差無し
 */
bool NoiseGenerator::IsOverRiver(const FVector2f &Point, const FVector2f &Vector, const FIntPoint &SourceVertex, const TSet<FIntPoint> &TargetVertices) {
	// 次の頂点を取得して範囲外なら終了
	const FIntPoint NextVertex(GetVerConnection(SourceVertex));
	if(NextVertex == SourceVertex || !TargetVertices.Contains(NextVertex)) return false;
	// 次の次の頂点を取得して範囲外なら終了
	const FIntPoint AfterNextVertex(GetVerConnection(NextVertex));
	if(AfterNextVertex == NextVertex || !TargetVertices.Contains(AfterNextVertex)) return false;
	// 各基準点の設定
	const FVector2f ControlPoint(GetVerLocation(NextVertex));
	const FVector2f StartPoint(MPoint(FVector2f(GetVerLocation(SourceVertex)), ControlPoint));
	const FVector2f EndPoint(MPoint(FVector2f(GetVerLocation(AfterNextVertex)), ControlPoint));
	// 交差を判定して返戻
	return IsOverBezierCurve(Point, Vector, StartPoint, ControlPoint, EndPoint) || IsOverRiver(Point, Vector, NextVertex, TargetVertices);
}

/**
 * 二点の中点を取得する
 * @param PointA 一つ目の点の座標
 * @param PointB 二つ目の点の座標
 * @return 二点の中点
 */
FVector2f NoiseGenerator::MPoint(const FVector2f &PointA, const FVector2f &PointB) {
	// 中点の返戻
	return (PointA + PointB) * 0.5F;
}

/**
 * 点から線分迄の最短ベクトルを取得する
 * @param Point 点の座標
 * @param StartPoint 線分の始点座標
 * @param EndPoint 線分の終点座標
 * @return 点から線分の最短ベクトル
 */
FVector2f NoiseGenerator::GetVecToLineSegment(const FVector2f &Point, const FVector2f &StartPoint, const FVector2f &EndPoint) {
	// 基準のベクトル
	const FVector2f VectorA(EndPoint - StartPoint), VectorB(Point - StartPoint);
	// 最短ベクトルの返戻
	return VectorA * FMath::Clamp(FVector2f::DotProduct(VectorA, VectorB) / VectorA.SizeSquared(), 0.0F, 1.0F) - VectorB;
}

//float Cbrt(float x) {return x > 0.0F ? FMath::Pow(x, 1.0F / 3.0F) : -FMath::Pow(-x, 1.0F / 3.0F);}

/**
 * 点からベジェ曲線迄の最短ベクトルを取得する
 * https://www.desmos.com/calculator/wvgvn9tkqs
 * @param Point 点の座標
 * @param StartPoint ベジェ曲線の始点座標
 * @param ControlPoint ベジェ曲線の制御点座標
 * @param EndPoint ベジェ曲線の終点座標
 * @param AcceptableError 媒介変数の最大許容誤差
 * @return 点からベジェ曲線の最短ベクトル
 */
FVector2f NoiseGenerator::GetVecToBezierCurve(const FVector2f &Point, const FVector2f &StartPoint, const FVector2f &ControlPoint, const FVector2f &EndPoint, const float &Tolerance) {
	// 基準のベクトル
	const FVector2f VectorA(StartPoint - ControlPoint * 2.0F + EndPoint), VectorB(ControlPoint - StartPoint), VectorC(StartPoint - Point);
	// 距離算出方程式の係数
	const float a = VectorA.SizeSquared();
	const float b = FVector2f::DotProduct(VectorA, VectorB) * 3.0F;
	const float c = FVector2f::DotProduct(VectorA, VectorC) + VectorB.SizeSquared() * 2.0F;
	const float d = FVector2f::DotProduct(VectorB, VectorC);
	// 媒介変数の初期値の設定
	const float Inflection = a ? b / (a * -3.0F) : 1.0F; // 変曲点
	bool ParamDef = Inflection <= 0.0F || Inflection < 1.0 && Inflection * (Inflection * (Inflection * a + b) + c) < -d; // 媒介変数初期値
	if(ParamDef ? a + b + c + d < 0.0F : d > 0.0F) ParamDef ^= true;
	// ニュートン法による媒介変数の導出
	float Param = ParamDef, ParamTmp; // 媒介変数
	do {
		const float SecondDeriv = Param * (Param * a * 3.0F + b * 2.0F) + c; // ２階微分
		if(SecondDeriv <= 0.0F) break;
		const float FirstDeriv = Param * (Param * (Param * a + b) + c) + d; // １階微分
		if(FirstDeriv <= 0.0F == ParamDef) break;
		ParamTmp = Param;
		Param -= FirstDeriv / SecondDeriv;
	} while(Param == (Param = FMath::Clamp(Param, 0.0F, 1.0F)) && FMath::Abs(ParamTmp - Param) > Tolerance);
	// もう一方の初期値を媒介変数にした場合との距離の比較
	if(Param * (Param * (Param * (Param * a * 0.75F + b) + c * 1.5F) + d * 3.0F) > (ParamDef ? 0.0F : a * 0.75F + b + c * 1.5 + d * 3.0F)) Param = !ParamDef;
	// 最短ベクトルの返戻
	return VectorA * (Param * Param) + VectorB * (Param * 2.0F) + VectorC;
/*	// デバッグ用ニュートン法不使用版
	const float A = b / a;
	const float B = c / a;
	const float C = d / a;
	const float p = B - A * A / 3.0F;
	const float q = A * A * A * 2.0F / 27.0F - A * B / 3.0F + C;
	const float D = q * q / 4.0F + p * p * p / 27.0F;
	FVector2f ParamAns(0.0F, FLT_MAX);
	if(D > 0.0F) ParamAns.X = FMath::Clamp(Cbrt(-q * 0.5F + FMath::Sqrt(D)) + Cbrt(-q * 0.5F - FMath::Sqrt(D)) - A / 3.0F, 0.0F, 1.0F);
	else {
		const float Theta = FMath::Atan2(FMath::Sqrt(-D), -q * 0.5F);
		for(const float AnsNum : {0.0F, 1.0F, 2.0F}) {
			const float Param = FMath::Clamp(FMath::Sqrt(-p / 3.0F) * FMath::Cos((Theta + UE_float_TWO_PI * AnsNum) / 3.0F) * 2.0F - A / 3.0F, 0.0F, 1.0F);
			const float NewDist = Param * (Param * (Param * (Param * a * 0.75F + b) + c * 1.5F) + d * 3.0F);
			if(NewDist < ParamAns.Y) ParamAns = {Param, NewDist};
		}
	}
	return VectorA * (ParamAns.X * ParamAns.X) + VectorB * (ParamAns.X * 2.0F) + VectorC;*/
}

/**
 * ベクトルのベジェ曲線との交差を判定する
 * @param Point ベクトルの始点座標
 * @param Vector ベクトル
 * @param StartPoint ベジェ曲線の始点座標
 * @param ControlPoint ベジェ曲線の制御点座標
 * @param EndPoint ベジェ曲線の終点座標
 * @return true: 交差有り, false: 交差無し
 */
bool NoiseGenerator::IsOverBezierCurve(const FVector2f &Point, const FVector2f &Vector, const FVector2f &StartPoint, const FVector2f &ControlPoint, const FVector2f &EndPoint) {
	// 基準のベクトル
	const FVector2f VectorA(StartPoint - ControlPoint * 2.0F + EndPoint), VectorB(ControlPoint - StartPoint), VectorC(StartPoint - Point);
	// 交差点媒介変数導出方程式の係数
	const float a = FVector2f::CrossProduct(VectorA, Vector);
	const float b = FVector2f::CrossProduct(VectorB, Vector);
	const float c = FVector2f::CrossProduct(VectorC, Vector);
	// 交差の検出
	const float D = FastSqrt(b * b - a * c);
	if(FMath::IsNaN(D)) return false; // 交差点が無ければfalseを返戻
	const TArray<float> Formula(a ? TArray<float>({(-b + D) / a, (-b - D) / a}) : TArray<float>({c * 0.5F / b}));
	for(const float CurveParam : Formula) if(CurveParam >= 0.0F && CurveParam <= 1.0F) {
		const float LineParam = FMath::Abs(Vector.X) > FMath::Abs(Vector.Y)
			? (CurveParam * (CurveParam * VectorA.X + VectorB.X * 2.0F) + VectorC.X) / Vector.X
			: (CurveParam * (CurveParam * VectorA.Y + VectorB.Y * 2.0F) + VectorC.Y) / Vector.Y;
		if(LineParam >= 0.0F && LineParam <= 1.0625F) return true; // 交差点が線分上にあればtrueを返戻（誤差対応の為，ベクトルを1/16延長）
	}
	// 終了
	return false;
}

/**
 * 平方根の近似値を算出する
 * https://takashiijiri.com/study/miscs/fastsqrt.html
 * @param Value 値
 * @return 平方根
 */
float NoiseGenerator::FastSqrt(const float &Value) {
	// 引数が負の場合
	if(Value < 0.0F) return NAN;
	// ニュートン法による導出
	const float ValueHalf = Value * 0.5F;
	const int32 Tmp = 0x5F3759DF - (*(int32 *)&Value >> 1); // 初期値
	float ValueRes = *(float *)&Tmp;
	for(int32 i = 0; i < 2; ++i) ValueRes *= 1.5F - ValueRes * ValueRes * ValueHalf;
	// 平方根の返戻
	return ValueRes * Value;
}