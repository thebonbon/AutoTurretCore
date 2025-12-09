class BON_AutoTurretTarget
{
	[SortAttribute()]
	float m_fDistance;

	IEntity m_Ent;
	RplId m_iRplId;
	PerceivableComponent m_PerceivableComp;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance)
	{
		m_Ent = ent;
		m_fDistance = distance;

		m_PerceivableComp = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));
	}

	//------------------------------------------------------------------------------------------------
	vector GetAimPoint()
	{
		vector aimPoint = m_Ent.GetOrigin();
		if (m_PerceivableComp)
		{
			array<vector> aimPoints();
			m_PerceivableComp.GetAimpoints(aimPoints); //World pos
			if (!aimPoints.IsEmpty())
				aimPoint = aimPoints[0];
		}

		return aimPoint;
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
