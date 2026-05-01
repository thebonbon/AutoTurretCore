modded class SCR_Math3D
{
	//------------------------------------------------------------------------------------------------
	static float WrapAngleDiffDeg(float angle)
	{
		while (angle < -180)
			angle += 360;
	
		while (angle > 180)
			angle -= 360;
	
		return angle;
	}

	//------------------------------------------------------------------------------------------------
	static vector GetLocalAngles(vector rotationMat[4], vector fromPos, vector toPos)
	{
		vector dirToTarget = vector.Direction(fromPos, toPos).Normalized();
	
		if (float.AlmostEqual(dirToTarget.LengthSq(), 0))
			return vector.Zero;
	
		float fromQuat[4];
		Math3D.MatrixToQuat(rotationMat, fromQuat);
	
		float fromQuatInv[4];
		Math3D.QuatInverse(fromQuatInv, fromQuat);
	
		vector dirLocal = SCR_Math3D.QuatMultiply(fromQuatInv, dirToTarget);
		
		return dirLocal.Normalized().VectorToAngles().MapAngles();
	}
	
	//------------------------------------------------------------------------------------------------
	static vector GetRandomVector(float min, float max)
	{
		vector result;
		result[0] = s_AIRandomGenerator.RandFloatXY(min, max);
		result[1] = s_AIRandomGenerator.RandFloatXY(min, max);
		result[2] = s_AIRandomGenerator.RandFloatXY(min, max);

		return result;
	}

	//------------------------------------------------------------------------------------------------
	static void AddRandomVectorToMat(inout vector mat[4], float min, float max)
	{
		vector randomVector = SCR_Math3D.GetRandomVector(min, max);
		vector randomMat[3];
		Math3D.AnglesToMatrix(randomVector, randomMat);
		Math3D.MatrixMultiply3(mat, randomMat, mat);
	}
}
