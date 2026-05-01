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
		//Pitch
		vector dirToTarget = vector.Direction(barrelPos, targetPos);
	
		float bodyQuat[4];
		Math3D.MatrixToQuat(bodyMat, bodyQuat);
	
		float bodyQuatInv[4];
		Math3D.QuatInverse(bodyQuatInv, bodyQuat);
	
		vector dirLocal = SCR_Math3D.QuatMultiply(bodyQuatInv, dirToTarget);
	
		vector localAngles = dirLocal.Normalized().VectorToAngles();
	
		float pitch = localAngles[1];
		pitch = SCR_Math3D.WrapAngleDiffDeg(pitch);
		
		//Yaw
		vector bodyPos = bodyMat[3];
		targetPos[1] = bodyPos[1];
	
		vector flatDir = vector.Direction(bodyPos, targetPos).Normalized();
	
		float yaw = flatDir.VectorToAngles()[0];
		yaw = SCR_Math3D.WrapAngleDiffDeg(yaw);
		
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
