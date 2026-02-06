modded class SCR_Math3D
{
	//------------------------------------------------------------------------------------------------
	static vector LerpAngle(vector current, vector target, float maxStep)
	{
		vector result;
		for (int i = 0; i < 3; i++)
		{
			result[i] = SCR_Math.LerpAngle(current[i], target[i], Math.Min(maxStep, 1));
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	static vector GetLocalAngles(vector fromMat[4], vector toPos)
	{
		vector dirWorld = vector.Direction(fromMat[3], toPos);
		dirWorld.Normalize();

		vector dirLocal;
		Math3D.MatrixInvMultiply3(fromMat, dirWorld, dirLocal);

		return dirLocal.VectorToAngles().MapAngles();
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
