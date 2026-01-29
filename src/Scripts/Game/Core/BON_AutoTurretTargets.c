class BON_AutoTurretTarget
{
	[SortAttribute()]
	float m_fDistance; //Only used for target sorting

	IEntity m_Ent;
	PerceivableComponent m_PerceivableComp;
	BON_AutoTurretTargetComponent m_TargetComp;
	int m_iFactionID = -1;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance = 0)
	{
		m_Ent = ent;
		m_fDistance = distance;

		m_PerceivableComp = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));
		m_TargetComp = BON_AutoTurretTargetComponent.Cast(ent.FindComponent(BON_AutoTurretTargetComponent));
		
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (factionComp)
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			m_iFactionID = factionManager.GetFactionIndex(factionComp.GetAffiliatedFaction());
		}
		else //e.g missiles dont have faction comp -> try target data
		{
			if (m_TargetComp)
				m_iFactionID = m_TargetComp.m_iFactionID;
		}
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

	//------------------------------------------------------------------------------------------------
	void SetAlarm(int pitch)
	{
		if (m_TargetComp)
			m_TargetComp.SetAlarm(pitch);
	}

	//------------------------------------------------------------------------------------------------
	//! Target instance no longer used by any turret -> stop sound
	void ~BON_AutoTurretTarget()
	{
		if (m_TargetComp)
			m_TargetComp.StopAlarm();
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

			BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(candidate, distance);
			if (newTarget.IsValid())
				sortedTargets.Insert(newTarget);
		}

		sortedTargets.Sort(); //Sorted by m_fDistance, smallest first

		return sortedTargets.Count();
	}
}
