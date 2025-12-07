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
	static vector GetLocalAngles(IEntity from, IEntity to)
	{
		vector dirWorld = to.GetOrigin() - from.GetOrigin();
		dirWorld.Normalize();
				
		vector mat[3];
		from.GetTransform(mat);
		
		vector dirLocal;
		Math3D.MatrixInvMultiply3(mat, dirWorld, dirLocal);
		
		return dirLocal.VectorToAngles().MapAngles();
	}
}