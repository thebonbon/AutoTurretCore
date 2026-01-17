class BON_AutoTurretTarget
{
	[SortAttribute()]
	float m_fDistance; //Only used for target sorting

	IEntity m_Ent;
	RplId m_iRplId;
	PerceivableComponent m_PerceivableComp;
	int m_iFactionID = -1;

	//------------------------------------------------------------------------------------------------
	void BON_AutoTurretTarget(IEntity ent, float distance, int factionID)
	{
		m_Ent = ent;
		m_fDistance = distance;
		m_iFactionID = factionID;

		m_PerceivableComp = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));
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
	static bool Extract(BON_AutoTurretTarget instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iFactionID);		
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, BON_AutoTurretTarget instance)
	{
		snapshot.SerializeInt(instance.m_iRplId);
		snapshot.SerializeInt(instance.m_iFactionID);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);		// m_iRplId
		snapshot.EncodeInt(packet);		// m_iFactionID
	}

	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);		// m_iRplId
		snapshot.DecodeInt(packet);		// m_iFactionID
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, 8); //2x 4bit ints
	}

	//------------------------------------------------------------------------------------------------
	static bool PropCompare(BON_AutoTurretTarget instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iRplId)
			&& snapshot.CompareInt(instance.m_iFactionID);
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
			BON_AutoTurretTargetComponent targetComp = BON_AutoTurretTargetComponent.Cast(candidate.FindComponent(BON_AutoTurretTargetComponent));
			
			BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(candidate, distance, targetComp.m_iFactionID);
			if (newTarget.IsValid())
				sortedTargets.Insert(newTarget);
		}

		sortedTargets.Sort(); //Sorted by m_fDistance, smallest first

		return sortedTargets.Count();
	}
}
