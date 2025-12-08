class BON_AutoTurretTarget
{
	[SortAttribute()]
	float m_fDistance;
	
	IEntity m_Ent;
	RplId m_iRplId;
	vector m_vAimpoint;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance)
	{
		m_Ent = ent;
		m_fDistance = distance;
		m_vAimpoint = ent.GetOrigin();
		
		PerceivableComponent perceivableComp = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));
		if (perceivableComp)
		{
			array<vector> aimPoints();
			perceivableComp.GetAimpoints(aimPoints);
			if (!aimPoints.IsEmpty())
				m_vAimpoint = aimPoints[0];
		}
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetAimPoint()
	{
		return m_Ent.CoordToParent(m_vAimpoint);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns if entity was deleted or destroyed
	bool IsValid()
	{
		if (!m_Ent)
			return false;
		
		DamageManagerComponent dmgManager = DamageManagerComponent.Cast(m_Ent.FindComponent(DamageManagerComponent));
		if (!dmgManager || dmgManager.IsDestroyed())
			return false;

		return true;
	}
}

class BON_AutoTurretGridMap : PointGridMap
{
	//------------------------------------------------------------------------------------------------
	//! Finds all target in range and sorts by ascending distance. (Smaller first)
	int FindSortedTargetsInRage(IEntity owner, out notnull array<ref BON_AutoTurretTarget> sortedTargets, vector origin, float range, int mask = 0)
	{
		//Find all in range
		array<IEntity> entities = {};
		GetGame().GetAutoTurretGrid().FindEntitiesInRange(entities, origin, range, mask);

		//Sort by distance
		foreach (IEntity candidate : entities)
		{
			if (owner == candidate)
				continue;

			float distance = vector.DistanceSq(origin, candidate.GetOrigin());
			BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(candidate, distance);
			if (newTarget.IsValid())
				sortedTargets.Insert(newTarget);			
		}
		sortedTargets.Sort(); //Sorted by distance, smallest first
		
		return sortedTargets.Count();
	}
}
