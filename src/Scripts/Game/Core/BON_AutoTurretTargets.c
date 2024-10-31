class BON_AutoTurretTargets
{	
	static ref array<IEntity> s_aTargetCharacters = {};
	static ref array<IEntity> s_aTargetVehicles = {};
	static ref array<IEntity> s_aTargetAircrafts = {};
	static ref array<IEntity> s_aTargetProjectiles = {};
}

enum BON_TurretTargetFilterFlags
{
	CHARACTERS = 1 << 0,
	VEHICLES = 1 << 1,
	AIRCRAFTS = 1 << 2,
	PROJECTILES = 1 << 3,
}

class BON_AutoTurretTarget
{
	IEntity m_Ent;
	float m_fDistance;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance)
	{
		m_Ent = ent;
		m_fDistance = distance;
	}
}
