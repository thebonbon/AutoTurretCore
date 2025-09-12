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

class BON_AutoTurretGridMap : PointGridMap
{
	//------------------------------------------------------------------------------------------------
	//! Finds all target in range and sorts by ascending distance. (Smaller first)
	int FindSortedTargetsInRage(out notnull array<ref BON_AutoTurretTarget> sortedEntities, vector origin, float range, int mask = 0)
	{
		//Find all in range
		GetGame().GetAutoTurretGrid().Update();
		array<IEntity> entities = {};
		int targetCount = GetGame().GetAutoTurretGrid().FindEntitiesInRange(entities, origin, range, mask);

		//Sort by distance
		foreach (IEntity target : entities)
		{
			float distance = vector.DistanceSq(origin, target.GetOrigin());
			BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(target, distance);

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
