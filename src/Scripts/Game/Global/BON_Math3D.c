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
	static vector ComputeTargetAngles(vector bodyMat[4], vector barrelPos, vector targetPos)
	{
		vector dir = vector.Direction(barrelPos, targetPos).Normalized();
	
		float localX = vector.Dot(dir, bodyMat[0]);
		float localY = vector.Dot(dir, bodyMat[1]);
		float localZ = vector.Dot(dir, bodyMat[2]);
	
		vector dirLocal = Vector(localX, localY, localZ).Normalized();	
		vector angles = dirLocal.VectorToAngles();
	
		float yaw = SCR_Math3D.WrapAngleDiffDeg(angles[0]);
		float pitch = SCR_Math3D.WrapAngleDiffDeg(angles[1]);
	
		return Vector(yaw, pitch, 0);
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
