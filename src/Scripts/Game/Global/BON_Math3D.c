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
}