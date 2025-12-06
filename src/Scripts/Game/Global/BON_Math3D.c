modded class SCR_Math3D
{
	//------------------------------------------------------------------------------------------------
	static vector AnglesFromTo(vector start, vector target)
	{
		vector dir = target - start;
		vector angles = dir.VectorToAngles();	
		return angles;
		//return SCR_AIPolar.WrapAngle(angles);
	}
}