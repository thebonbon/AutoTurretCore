class BON_AutoTurretTarget
{
	IEntity m_Ent;
	RplId m_iRplId;
	float m_fDistance;
	vector m_vAimpoint;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance)
	{
		m_Ent = ent;
		m_fDistance = distance;
	}

	//------------------------------------------------------------------------------------------------
	bool IsValid()
	{
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
	int FindSortedTargetsInRage(IEntity owner, out notnull array<ref BON_AutoTurretTarget> sortedEntities, vector origin, float range, int mask = 0)
	{
		//Find all in range
		GetGame().GetAutoTurretGrid().Update();
		array<IEntity> entities = {};
		int targetCount = GetGame().GetAutoTurretGrid().FindEntitiesInRange(entities, origin, range, mask);

		//Sort by distance
		foreach (IEntity target : entities)
		{
			if (owner == target)
				continue;

			float distance = vector.DistanceSq(origin, target.GetOrigin());
			BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(target, distance);
			if (!newTarget.IsValid())
				continue;

			//First entry
			if (sortedEntities.IsEmpty())
			{
				sortedEntities.Insert(newTarget);
				continue;
			}

			//Insert before
			bool inserted = false;
			foreach (int i, BON_AutoTurretTarget oldTarget : sortedEntities)
			{
				if (distance < oldTarget.m_fDistance)
				{
					sortedEntities.InsertAt(newTarget, i);
					inserted = true;
					break;
				}
			}
			//Insert last
			if (!inserted)
				sortedEntities.Insert(newTarget);
		}
		return targetCount;
	}
}
