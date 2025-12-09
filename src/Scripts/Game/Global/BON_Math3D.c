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
		vector dirWorld = toPos - fromMat[3];
		dirWorld.Normalize();
		
		vector dirLocal;
		Math3D.MatrixInvMultiply3(fromMat, dirWorld, dirLocal);
		
		return dirLocal.VectorToAngles().MapAngles();
	}
}