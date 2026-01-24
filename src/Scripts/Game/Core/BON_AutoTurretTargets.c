class BON_AutoTurretTarget
{
	[SortAttribute()]
	float m_fDistance; //Only used for target sorting

	IEntity m_Ent;
	RplId m_iRplId = -1;
	PerceivableComponent m_PerceivableComp;
	int m_iFactionID = -1;

	//------------------------------------------------------------------------------------------------
	static BON_AutoTurretTarget Create(IEntity ent, float distance = 0, int factionId = -1)
	{
		BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget();
		newTarget.m_fDistance = distance;
		newTarget.m_iFactionID = factionId;
		
		newTarget.CreateFromEnt(ent);

		return newTarget;
	}

	//------------------------------------------------------------------------------------------------
	void CreateFromEnt(IEntity ent)
	{
		m_Ent = ent;

		if (!ent)
			return;

		m_Ent = ent;

		BON_AutoTurretTargetComponent targetComp = BON_AutoTurretTargetComponent.Cast(ent.FindComponent(BON_AutoTurretTargetComponent));
		m_iFactionID = targetComp.m_iFactionID;

		m_PerceivableComp = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));

		RplComponent rplComp = RplComponent.Cast(ent.FindComponent(RplComponent));
		m_iRplId = rplComp.Id()
	}

	//------------------------------------------------------------------------------------------------
	vector GetAimPoint()
	{
		if (!m_Ent)
			return vector.Zero;

		vector aimPoint = m_Ent.GetOrigin();

		if (m_PerceivableComp)
		{
			array<vector> aimPoints();
			int points = m_PerceivableComp.GetAimpoints(aimPoints); //World pos
			if (points > 0)
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

		foreach (IEntity candidate : entities)
		{
			if (owner == candidate)
				continue;

			float distance = vector.DistanceSq(origin, candidate.GetOrigin());

			BON_AutoTurretTarget newTarget = BON_AutoTurretTarget.Create(candidate, distance);
			if (newTarget.IsValid())
				sortedTargets.Insert(newTarget);
		}

		sortedTargets.Sort(); //Sorted by m_fDistance, smallest first

		return sortedTargets.Count();
	}
}
