[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetingComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetingComponent : ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.Flags, enumType: BON_TurretTargetFilterFlags, category: "Targeting")]
	BON_TurretTargetFilterFlags m_eTargetFlags;

	[Attribute("500", UIWidgets.Auto, "Radius to scan / attack targets (m)", category: "Targeting")]
	int m_iSearchRadius;

	[Attribute("2", UIWidgets.Auto, "Time to scan for new targets (s)", category: "Targeting")]
	protected float m_fMaxSearchTime;

	ref BON_AutoTurretTarget m_CurrentTarget;

	protected float m_fSearchTimer = 0;
	protected BON_AutoTurretAimingComponent m_AimingComp;
	protected FactionAffiliationComponent m_FactionComp
	protected BON_AutoTurretComponent m_MainTurretComp;

	//------------------------------------------------------------------------------------------------
	bool IsEnemy(BON_AutoTurretTarget target)
	{
		if (target.m_iFactionID == -1)
			return false;

		FactionManager factionManager = GetGame().GetFactionManager();
		Faction targetFaction = factionManager.GetFactionByIndex(target.m_iFactionID);

		return targetFaction.IsFactionEnemy(m_FactionComp.GetAffiliatedFaction());
	}

	//------------------------------------------------------------------------------------------------
	bool LineOfSightTo(BON_AutoTurretTarget target)
	{
		if (!target)
			return false;

		vector muzzleMat[4];
		m_AimingComp.GetBarrelTransform(muzzleMat);

		TraceParam param = new TraceParam();
		param.Start = muzzleMat[3];
		param.End = target.GetAimPoint();
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.VISIBILITY;
		param.Exclude = GetOwner();
		param.LayerMask = EPhysicsLayerPresets.Projectile;
		float traceDistance = GetOwner().GetWorld().TraceMove(param, null);

		//Max distance or hit entity directly
		if (traceDistance == 1 || param.TraceEnt == target.m_Ent)
			return true;

		//Hit entity but its an equipment of the target (e.g vest, helmet etc..)
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(param.TraceEnt.FindComponent(InventoryItemComponent));
		return (itemComp && itemComp.GetParentSlot());
	}

	//------------------------------------------------------------------------------------------------
	//! Returns closest valid target from gridmap
	BON_AutoTurretTarget FindTarget()
	{
		array<ref BON_AutoTurretTarget> sortedTargets = {};
		GetGame().GetAutoTurretGrid().FindSortedTargetsInRage(GetOwner(), sortedTargets, GetOwner().GetOrigin(), m_iSearchRadius, m_eTargetFlags);

		foreach (BON_AutoTurretTarget target : sortedTargets)
		{
			if (m_AimingComp.IsWithinLimitsPos(target) && IsEnemy(target) && LineOfSightTo(target))
				return target;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	BON_AutoTurretTarget GetTarget()
	{
		return m_CurrentTarget;
	}

	//------------------------------------------------------------------------------------------------
	bool CleanupTarget()
	{
		m_CurrentTarget = null;
		Rpc(RpcDo_SetNewTarget, -1, -1);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SetNewTarget(RplId id, int factionId)
	{
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(id));
		if (!rplComponent)
		{
			m_CurrentTarget = null;
			return;
		}

		IEntity targetEnt = rplComponent.GetEntity();
		if (!targetEnt)
		{
			m_CurrentTarget = null;
			return;
		}

		m_CurrentTarget = BON_AutoTurretTarget.Create(targetEnt, 0, factionId);
	}

	//------------------------------------------------------------------------------------------------
	bool CheckTarget()
	{
		if (!m_CurrentTarget.IsValid())
			return CleanupTarget();

		if (!m_AimingComp.IsWithinLimitsPos(m_CurrentTarget))
			return CleanupTarget();

		if (!LineOfSightTo(m_CurrentTarget))
			return CleanupTarget();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_CurrentTarget && m_CurrentTarget.m_Ent)
			Shape.CreateArrow(owner.GetOrigin(), m_CurrentTarget.m_Ent.GetOrigin(), 0.1, COLOR_GREEN, ShapeFlags.ONCE);

		m_fSearchTimer -= timeSlice;
		if (m_fSearchTimer > 0)
			return;

		m_fSearchTimer = m_fMaxSearchTime;

		if (!m_CurrentTarget || !CheckTarget())
		{
			m_CurrentTarget = FindTarget();
			if (!m_CurrentTarget)
				return;

			RplComponent rplComp = RplComponent.Cast(m_CurrentTarget.m_Ent.FindComponent(RplComponent));
			Rpc(RpcDo_SetNewTarget, rplComp.Id(), m_CurrentTarget.m_iFactionID);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_MainTurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));
		m_AimingComp = BON_AutoTurretAimingComponent.Cast(owner.FindComponent(BON_AutoTurretAimingComponent));
		m_FactionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
