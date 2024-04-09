#include "NoiseGenerator.h"

/**
 * �R���X�g���N�^
 * @param Seed �V�[�h�l
 */
NoiseGenerator::NoiseGenerator(const int32 &Seed) {
	NoiseSeed = Seed;
	SimplexNoise = UFastNoise2BlueprintLibrary::MakeSimplexGenerator();
	return;
}

/**
 * �W�����Z�o����
 * @param Location ���Y�n�_�̕��ʍ��W
 * @return ���Y�n�_�̕W��
 */
FVector2f NoiseGenerator::CalculateElevation(const FVector2f &Location) {
	// �W���̎Z�o
	const float LandNoise = FMath::Cube(GetSimplexNoise(Location, 1.0F));
	const float MainNoise = FMath::Square(GetSimplexNoise(Location));
	const float MiniNoise = FMath::Square(GetSimplexNoise(Location * 128.0F, 64.0F));
	float Elevation = (LandNoise + MainNoise * SwellScale) * SimpScaleV; // �W��
	float WaterLevel = 0.0F;
	// ����̐���
	if(Elevation > 0.0F) { // �嗤
		// ���Y�n�_�̃Z���̈ʒu�̎擾
		const FVector2f ScaleLoc(Location / CellScaleH);
		const int32 OriginX = FMath::Floor(ScaleLoc.X) - 2, OriginY = FMath::Floor(ScaleLoc.Y) - 2;
		// �T���͈͓��̒��_�̎擾
		TSet<FIntPoint> TargetVertices;
		for(int32 X = OriginX; X < OriginX + 5; ++X) for(int32 Y = OriginY; Y < OriginY + 5; ++Y) TargetVertices.Add({X, Y});
		// �e�Ώۉ͐�̒T���͈͓��̍ŏ㗬�̒��_�̎擾
		TSet<FIntPoint> SourceVertices(TargetVertices);
		for(int32 X = OriginX; X < OriginX + 5; ++X) for(int32 Y = OriginY; Y < OriginY + 5; ++Y) SourceVertices.Remove(GetVerConnection({X, Y}));
		// �R���̑���
		const FVector2f PathDist(GetDistSquaredToPath(ScaleLoc, TargetVertices)); // �P�ԖڂƂQ�Ԗڂɋ߂��p�X���̓�拗��
		const float Add = PathDist.X + PathDist.Y;
		const float Sub = PathDist.Y - PathDist.X;
		const float Rnd = LandNoise * RidgeRound + 1.0F;
		WaterLevel = Elevation += (Add < Sub * Rnd ? PathDist.X : (Add * 2.0F - Sub * Sub * Rnd / Add - Add / Rnd) * 0.25F) * (LandNoise + MainNoise * SwellScale) * MainNoise * CellScaleV;
		// �͐�̑���
		const FVector2f RiverDist(GetDistSquaredToRiver(ScaleLoc, SourceVertices, TargetVertices)); // �P�ԖڂƂQ�Ԗڂɋ߂��͐얘�̓�拗��
		const float CorrectedElev = Elevation * RiverWidth;
		if(RiverDist.X * CorrectedElev < 1.0F) { // �͐� https://www.desmos.com/calculator/jzjsxtodsn
			const float RivSpacing = RiverDist.Y != FLT_MAX ? FastSqrt(RiverDist.X * RiverDist.Y) * 0.5F + (RiverDist.X + RiverDist.Y) * 0.25F : FLT_MAX;
			const float Riverbed = RivSpacing * CorrectedElev < 1.0F ? (FMath::Square(1.0F - RiverDist.X / RivSpacing) - 1.0F) * RivSpacing * CorrectedElev + 1.0F : FMath::Square(1.0F - RiverDist.X * CorrectedElev);
			const float ClimbRate = Elevation * 0.875F / (Elevation + RiverSlope);
			Elevation -= (1.0F - ClimbRate) * Riverbed * WaterDepth - (1.0F - Riverbed * ClimbRate) * (MainNoise + 1.0F) * MiniNoise * StoneScale;
		} else Elevation += (MainNoise + 1.0F) * MiniNoise * StoneScale; // ����
	//	if(RiverDist.X < 2.44140625E-4F) Elevation = 0.0F;
	} else { // �C�m
		Elevation -= WaterDepth - (MainNoise + 1.0F) * MiniNoise * StoneScale;
	}
	// �I��
	return FVector2f(WaterLevel, Elevation);
}

/**
 * ���_�̐ڑ�����擾����
 * @param Position ���_�̈ʒu
 * @return ���_�̐ڑ���
 */
FIntPoint NoiseGenerator::GetVerConnection(const FIntPoint &Position) {
	// �}�b�v�ɒ��_�̐ڑ��悪�L��ΕԖ�
	const FIntPoint *ConnectionPtr(VerConnection.Find(Position));
	if(ConnectionPtr) return *ConnectionPtr;
	// �}�b�v�ɖ�����Γ��o
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
	// �}�b�v�ɒ��_�̐ڑ����ǉ����ĕԖ�
	return VerConnection.Add(Position, Connection.Key);
}

/**
 * ���_�̍��W���擾����
 * @param Position ���_�̈ʒu
 * @return ���_�̍��W
 */
FVector3f NoiseGenerator::GetVerLocation(const FIntPoint &Position) {
	// �}�b�v�ɒ��_�̍��W���L��ΕԖ�
	const FVector3f *LocationPtr(VerLocation.Find(Position));
	if(LocationPtr) return *LocationPtr;
	// �}�b�v�ɖ�����Γ��o
	const int32 Idx = Hash(Position);
	const FVector2f Location2D(Position.X + RandVecs[Idx], Position.Y + RandVecs[Idx | 1]);
	//	const FVector2f Location2D(Position.X + RandVecs[Idx] * 0.75F, Position.Y + RandVecs[Idx | 1] * 0.75F);
	const FVector3f Location(Location2D, GetSimplexNoise(Location2D * CellScaleH, 1.0F));
	// �}�b�v�ɒ��_�̍��W��ǉ����ĕԖ�
	return VerLocation.Add(Position, Location);
}

/**
 * �n�b�V���l�𐶐�����
 * @param Position �Z���̈ʒu
 * @return �n�b�V���l
 */
int32 NoiseGenerator::Hash(const FIntPoint &Position) const {
	const int32 PrimeX = 501125321, PrimeY = 1136930381;
	int32 hash = NoiseSeed ^ Position.X * PrimeX ^ Position.Y * PrimeY;
	hash *= 0x27d4eb2d;
	return hash & (255 << 1);
}

/**
 * �V���v���b�N�X�m�C�Y�̒l���擾����
 * @param Location �m�C�Y�擾���W
 * @param MaxScale
 * @return �W��
 */
float NoiseGenerator::GetSimplexNoise(const FVector2f &Location, const float &MaxScale) {
	float Noise = 0.0F;
	const FVector2D ScaleLoc(Location / SimpScaleH);
	for(float Scale = 1.0F; Scale <= MaxScale; Scale *= 2.0F) Noise += SimplexNoise->GenSingle2D(ScaleLoc * Scale, NoiseSeed) / Scale;
	return Noise;
}

/**
 * �P�ԖڂƂQ�Ԗڂɋ߂��p�X���̓�拗�����擾����
 * @param Point �_�̍��W
 * @param TargetVertices �T���͈͓��̒��_
 * @return �P�ԖڂƂQ�Ԗڂɋ߂��p�X���̋����iX: �P�Ԗ�, Y: �Q�Ԗځj
 */
FVector2f NoiseGenerator::GetDistSquaredToPath(const FVector2f &Point, const TSet<FIntPoint> &TargetVertices) {
	// �x�N�g�����i�[����}�b�v
	TMap<FIntPoint, TPair<FVector2f, float>> Vectors;
	// �p�X�����̋������擾���ă}�b�v�ɒǉ����ċ߂����Ƀ\�[�g
	for(const FIntPoint &Vertex : TargetVertices) {
		const FVector2f Vector(GetVecToLineSegment(Point, FVector2f(GetVerLocation(Vertex)), FVector2f(GetVerLocation(GetVerConnection(Vertex)))));
		Vectors.Add(Vertex, {Vector, Vector.SizeSquared()});
	}
	Vectors.ValueSort([](const TPair<FVector2f, float> &A, const TPair<FVector2f, float> &B) {return A.Value < B.Value; });
	// �P�ԖڂƂQ�Ԗڂɋ߂��p�X���̋����̎擾
	bool First = true;
	FVector2f MinVector, Distances(FLT_MAX);
	for(const TPair<FIntPoint, TPair<FVector2f, float>> &Vector : Vectors) if(First) {
		First = false;
		MinVector = Vector.Value.Key;
		Distances.X = Vector.Value.Value; // �P�Ԗڂɋ߂��p�X���̋���
	} else if(FVector2f::DotProduct(MPoint(FVector2f(GetVerLocation(Vector.Key)), FVector2f(GetVerLocation(GetVerConnection(Vector.Key)))) - Point, MinVector) < Distances.X) {
		Distances.Y = Vector.Value.Value; // �Q�Ԗڂɋ߂��p�X���̋���
		return Distances;
	}
	// �P�ԖڂƂQ�Ԗڂɋ߂��p�X���̋����̕Ԗ�
	return Distances;
}

/**
 * �P�ԖڂƂQ�Ԗڂɋ߂��͐얘�̓�拗�����擾����
 * @param Point �_�̍��W
 * @param SourceVertices �e�͐�̒T���͈͓��̍ŏ㗬�̒��_
 * @param TargetVertices �T���͈͓��̒��_
 * @return �P�ԖڂƂQ�Ԗڂɋ߂��͐얘�̋����iX: �P�Ԗ�, Y: �Q�Ԗځj
 */
FVector2f NoiseGenerator::GetDistSquaredToRiver(const FVector2f &Point, const TSet<FIntPoint> &SourceVertices, const TSet<FIntPoint> &TargetVertices) {
	// �x�N�g�����i�[����}�b�v
	TMap<FIntPoint, TPair<FVector2f, float>> Vectors, MinVectors;
	// �͐쒆�S�����̋������擾���ă}�b�v�ɒǉ����ċ߂����Ƀ\�[�g
	for(const FIntPoint &Vertex : SourceVertices) MinVectors.Add(Vertex, GetVecToRiver(Point, Vertex, TargetVertices, Vectors));
	MinVectors.ValueSort([](const TPair<FVector2f, float> &A, const TPair<FVector2f, float> &B) {return A.Value < B.Value; });
	// �P�ԖڂƂQ�Ԗڂɋ߂��͐얘�̋����̎擾
	bool First = true;
	FIntPoint SourceVertex;
	FVector2f MinVector, Distances(FLT_MAX);
	for(const TPair<FIntPoint, TPair<FVector2f, float>> &Vector : MinVectors) if(First) {
		First = false;
		SourceVertex = Vector.Key;
		MinVector = Vector.Value.Key;
		Distances.X = Vector.Value.Value; // �P�Ԗڂɋ߂��͐얘�̋���
	} else if(Vector.Value.Key != MinVector && !IsOverRiver(Point, Vector.Value.Key, SourceVertex, TargetVertices)) {
		Distances.Y = Vector.Value.Value; // �Q�Ԗڂɋ߂��͐얘�̋���
		return Distances;
	}
	// �P�ԖڂƂQ�Ԗڂɋ߂��͐얘�̋����̕Ԗ�
	return Distances;
}

/**
 * �_����͐쒆�S�����̍ŒZ�x�N�g�����擾����
 * @param Point �_�̍��W
 * @param SourceVertex �Ώۉ͐�̒T���͈͓��̍ŏ㗬�̒��_
 * @param TargetVertices �T���͈͓��̒��_
 * @param Vectors �͐쒆�S���̊e��Ԗ��̍ŒZ�x�N�g�����i�[����}�b�v
 * @return �_����͐쒆�S���̍ŒZ�x�N�g��
 */
TPair<FVector2f, float> NoiseGenerator::GetVecToRiver(const FVector2f &Point, const FIntPoint &SourceVertex, const TSet<FIntPoint> &TargetVertices, TMap<FIntPoint, TPair<FVector2f, float>> &Vectors) {
	// �}�b�v�ɉ͐쒆�S�����̃x�N�g�����L��ΕԖ�
	const TPair<FVector2f, float> *VectorPtr(Vectors.Find(SourceVertex));
	if(VectorPtr) return *VectorPtr;
	// ���̒��_���擾���Ĕ͈͊O�Ȃ�I��
	const FIntPoint NextVertex(GetVerConnection(SourceVertex));
	if(NextVertex == SourceVertex || !TargetVertices.Contains(NextVertex)) return Vectors.Add(SourceVertex, {FVector2f::ZeroVector, FLT_MAX});
	// ���̎��̒��_���擾���Ĕ͈͊O�Ȃ�I��
	const FIntPoint AfterNextVertex(GetVerConnection(NextVertex));
	if(AfterNextVertex == NextVertex || !TargetVertices.Contains(AfterNextVertex)) return Vectors.Add(SourceVertex, {FVector2f::ZeroVector, FLT_MAX});
	// �e��_�̐ݒ�
	const FVector2f ControlPoint(GetVerLocation(NextVertex));
	const FVector2f StartPoint(MPoint(FVector2f(GetVerLocation(SourceVertex)), ControlPoint));
	const FVector2f EndPoint(MPoint(FVector2f(GetVerLocation(AfterNextVertex)), ControlPoint));
	// �͐쒆�S�����̃x�N�g���𓱏o
	const FVector2f Vector(GetVecToBezierCurve(Point, StartPoint, ControlPoint, EndPoint));
	const float Distance = Vector.SizeSquared();
	// �}�b�v�Ɍ��݂̒��_����͈͓��̍ŉ������̍ŏ��l��ǉ����ĕԖ�
	const TPair<FVector2f, float> Downstream(GetVecToRiver(Point, NextVertex, TargetVertices, Vectors));
	return Vectors.Add(SourceVertex, Distance < Downstream.Value ? TPair<FVector2f, float>(Vector, Distance) : Downstream);
}

/**
 * �x�N�g���̉͐쒆�S���Ƃ̌����𔻒肷��
 * @param Point �x�N�g���̎n�_���W
 * @param Vector �x�N�g��
 * @param SourceVertex �Ώۉ͐�̒T���͈͓��̍ŏ㗬�̒��_
 * @param TargetVertices �T���͈͓��̒��_
 * @return true: �����L��, false: ��������
 */
bool NoiseGenerator::IsOverRiver(const FVector2f &Point, const FVector2f &Vector, const FIntPoint &SourceVertex, const TSet<FIntPoint> &TargetVertices) {
	// ���̒��_���擾���Ĕ͈͊O�Ȃ�I��
	const FIntPoint NextVertex(GetVerConnection(SourceVertex));
	if(NextVertex == SourceVertex || !TargetVertices.Contains(NextVertex)) return false;
	// ���̎��̒��_���擾���Ĕ͈͊O�Ȃ�I��
	const FIntPoint AfterNextVertex(GetVerConnection(NextVertex));
	if(AfterNextVertex == NextVertex || !TargetVertices.Contains(AfterNextVertex)) return false;
	// �e��_�̐ݒ�
	const FVector2f ControlPoint(GetVerLocation(NextVertex));
	const FVector2f StartPoint(MPoint(FVector2f(GetVerLocation(SourceVertex)), ControlPoint));
	const FVector2f EndPoint(MPoint(FVector2f(GetVerLocation(AfterNextVertex)), ControlPoint));
	// �����𔻒肵�ĕԖ�
	return IsOverBezierCurve(Point, Vector, StartPoint, ControlPoint, EndPoint) || IsOverRiver(Point, Vector, NextVertex, TargetVertices);
}

/**
 * ��_�̒��_���擾����
 * @param PointA ��ڂ̓_�̍��W
 * @param PointB ��ڂ̓_�̍��W
 * @return ��_�̒��_
 */
FVector2f NoiseGenerator::MPoint(const FVector2f &PointA, const FVector2f &PointB) {
	// ���_�̕Ԗ�
	return (PointA + PointB) * 0.5F;
}

/**
 * �_����������̍ŒZ�x�N�g�����擾����
 * @param Point �_�̍��W
 * @param StartPoint �����̎n�_���W
 * @param EndPoint �����̏I�_���W
 * @return �_��������̍ŒZ�x�N�g��
 */
FVector2f NoiseGenerator::GetVecToLineSegment(const FVector2f &Point, const FVector2f &StartPoint, const FVector2f &EndPoint) {
	// ��̃x�N�g��
	const FVector2f VectorA(EndPoint - StartPoint), VectorB(Point - StartPoint);
	// �ŒZ�x�N�g���̕Ԗ�
	return VectorA * FMath::Clamp(FVector2f::DotProduct(VectorA, VectorB) / VectorA.SizeSquared(), 0.0F, 1.0F) - VectorB;
}

//float Cbrt(float x) {return x > 0.0F ? FMath::Pow(x, 1.0F / 3.0F) : -FMath::Pow(-x, 1.0F / 3.0F);}

/**
 * �_����x�W�F�Ȑ����̍ŒZ�x�N�g�����擾����
 * https://www.desmos.com/calculator/wvgvn9tkqs
 * @param Point �_�̍��W
 * @param StartPoint �x�W�F�Ȑ��̎n�_���W
 * @param ControlPoint �x�W�F�Ȑ��̐���_���W
 * @param EndPoint �x�W�F�Ȑ��̏I�_���W
 * @param AcceptableError �}��ϐ��̍ő勖�e�덷
 * @return �_����x�W�F�Ȑ��̍ŒZ�x�N�g��
 */
FVector2f NoiseGenerator::GetVecToBezierCurve(const FVector2f &Point, const FVector2f &StartPoint, const FVector2f &ControlPoint, const FVector2f &EndPoint, const float &Tolerance) {
	// ��̃x�N�g��
	const FVector2f VectorA(StartPoint - ControlPoint * 2.0F + EndPoint), VectorB(ControlPoint - StartPoint), VectorC(StartPoint - Point);
	// �����Z�o�������̌W��
	const float a = VectorA.SizeSquared();
	const float b = FVector2f::DotProduct(VectorA, VectorB) * 3.0F;
	const float c = FVector2f::DotProduct(VectorA, VectorC) + VectorB.SizeSquared() * 2.0F;
	const float d = FVector2f::DotProduct(VectorB, VectorC);
	// �}��ϐ��̏����l�̐ݒ�
	const float Inflection = a ? b / (a * -3.0F) : 1.0F; // �ϋȓ_
	bool ParamDef = Inflection <= 0.0F || Inflection < 1.0 && Inflection * (Inflection * (Inflection * a + b) + c) < -d; // �}��ϐ������l
	if(ParamDef ? a + b + c + d < 0.0F : d > 0.0F) ParamDef ^= true;
	// �j���[�g���@�ɂ��}��ϐ��̓��o
	float Param = ParamDef, ParamTmp; // �}��ϐ�
	do {
		const float SecondDeriv = Param * (Param * a * 3.0F + b * 2.0F) + c; // �Q�K����
		if(SecondDeriv <= 0.0F) break;
		const float FirstDeriv = Param * (Param * (Param * a + b) + c) + d; // �P�K����
		if(FirstDeriv <= 0.0F == ParamDef) break;
		ParamTmp = Param;
		Param -= FirstDeriv / SecondDeriv;
	} while(Param == (Param = FMath::Clamp(Param, 0.0F, 1.0F)) && FMath::Abs(ParamTmp - Param) > Tolerance);
	// ��������̏����l��}��ϐ��ɂ����ꍇ�Ƃ̋����̔�r
	if(Param * (Param * (Param * (Param * a * 0.75F + b) + c * 1.5F) + d * 3.0F) > (ParamDef ? 0.0F : a * 0.75F + b + c * 1.5 + d * 3.0F)) Param = !ParamDef;
	// �ŒZ�x�N�g���̕Ԗ�
	return VectorA * (Param * Param) + VectorB * (Param * 2.0F) + VectorC;
/*	// �f�o�b�O�p�j���[�g���@�s�g�p��
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
 * �x�N�g���̃x�W�F�Ȑ��Ƃ̌����𔻒肷��
 * @param Point �x�N�g���̎n�_���W
 * @param Vector �x�N�g��
 * @param StartPoint �x�W�F�Ȑ��̎n�_���W
 * @param ControlPoint �x�W�F�Ȑ��̐���_���W
 * @param EndPoint �x�W�F�Ȑ��̏I�_���W
 * @return true: �����L��, false: ��������
 */
bool NoiseGenerator::IsOverBezierCurve(const FVector2f &Point, const FVector2f &Vector, const FVector2f &StartPoint, const FVector2f &ControlPoint, const FVector2f &EndPoint) {
	// ��̃x�N�g��
	const FVector2f VectorA(StartPoint - ControlPoint * 2.0F + EndPoint), VectorB(ControlPoint - StartPoint), VectorC(StartPoint - Point);
	// �����_�}��ϐ����o�������̌W��
	const float a = FVector2f::CrossProduct(VectorA, Vector);
	const float b = FVector2f::CrossProduct(VectorB, Vector);
	const float c = FVector2f::CrossProduct(VectorC, Vector);
	// �����̌��o
	const float D = FastSqrt(b * b - a * c);
	if(FMath::IsNaN(D)) return false; // �����_���������false��Ԗ�
	const TArray<float> Formula(a ? TArray<float>({(-b + D) / a, (-b - D) / a}) : TArray<float>({c * 0.5F / b}));
	for(const float CurveParam : Formula) if(CurveParam >= 0.0F && CurveParam <= 1.0F) {
		const float LineParam = FMath::Abs(Vector.X) > FMath::Abs(Vector.Y)
			? (CurveParam * (CurveParam * VectorA.X + VectorB.X * 2.0F) + VectorC.X) / Vector.X
			: (CurveParam * (CurveParam * VectorA.Y + VectorB.Y * 2.0F) + VectorC.Y) / Vector.Y;
		if(LineParam >= 0.0F && LineParam <= 1.0625F) return true; // �����_��������ɂ����true��Ԗ߁i�덷�Ή��ׁ̈C�x�N�g����1/16�����j
	}
	// �I��
	return false;
}

/**
 * �������̋ߎ��l���Z�o����
 * https://takashiijiri.com/study/miscs/fastsqrt.html
 * @param Value �l
 * @return ������
 */
float NoiseGenerator::FastSqrt(const float &Value) {
	// ���������̏ꍇ
	if(Value < 0.0F) return NAN;
	// �j���[�g���@�ɂ�铱�o
	const float ValueHalf = Value * 0.5F;
	const int32 Tmp = 0x5F3759DF - (*(int32 *)&Value >> 1); // �����l
	float ValueRes = *(float *)&Tmp;
	for(int32 i = 0; i < 2; ++i) ValueRes *= 1.5F - ValueRes * ValueRes * ValueHalf;
	// �������̕Ԗ�
	return ValueRes * Value;
}