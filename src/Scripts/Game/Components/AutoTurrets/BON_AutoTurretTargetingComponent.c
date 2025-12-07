[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetingComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetingComponent : ScriptComponent
{
	
	[Attribute(uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Targeting")]
	BON_TurretTargetFilterFlags m_eTargetFlags;
	
	[Attribute("500", UIWidgets.Auto, "Radius to scan / attack targets (m)", category: "Targeting")]
	int m_iSearchRadius;
	
	[Attribute("1", UIWidgets.Auto, "Time to scan for new targets (s)", category: "Targeting")]
	protected float m_fMaxSearchTime;
	
	protected float m_fSearchTimer = 0;
	protected BON_AutoTurretAimingComponent m_AimingComp;
	protected Faction m_Faction;
	
	protected ref BON_AutoTurretTarget m_CurrentTarget;
	protected BON_AutoTurretComponent m_MainTurretComp;
	
	//------------------------------------------------------------------------------------------------
	bool IsEnemy(IEntity ent)
	{
		#ifdef WORKBENCH
		if (ent.GetName() == "BOB")
			return true;
		#endif

		if (Projectile.Cast(ent))
			return true;

		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction targetFaction = factionComp.GetAffiliatedFaction();
		if (!targetFaction)
			return false;

		return targetFaction.IsFactionEnemy(m_Faction);
	}

	//------------------------------------------------------------------------------------------------
	bool LineOfSightTo(BON_AutoTurretTarget target)
	{
		if (!target)
			return false;

		vector muzzleMat[4];
		m_MainTurretComp.GetCurrentMuzzle().GetTransform(muzzleMat);

		TraceParam param = new TraceParam();
		param.Start = muzzleMat[3];
		param.End = target.GetAimPoint();
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.VISIBILITY;
		param.Exclude = GetOwner();
		param.LayerMask = EPhysicsLayerPresets.Projectile;
		float traceDistance = GetOwner().GetWorld().TraceMove(param, null);

		return (traceDistance == 1) || (param.TraceEnt == target.m_Ent);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns closest valid target from gridmap
	BON_AutoTurretTarget FindTarget()
	{		
		array<ref BON_AutoTurretTarget> sortedTargets = {};
		GetGame().GetAutoTurretGrid().FindSortedTargetsInRage(GetOwner(), sortedTargets, GetOwner().GetOrigin(), m_iSearchRadius, m_eTargetFlags);

		foreach (BON_AutoTurretTarget target : sortedTargets)
		{
			//if (m_AimingComp.IsWithinLimits(target.GetAimPoint()) && LineOfSightTo(target))
			if (LineOfSightTo(target))
				return target;
		}

		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	bool CheckTarget()
	{
		if (!m_CurrentTarget.IsValid())
		{
			m_CurrentTarget = null;
			return false;
		}
		
		//Add limits again..
		bool valid = LineOfSightTo(m_CurrentTarget);
		if (valid)
			return true;
		
		m_CurrentTarget = null;
		return false;		
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_CurrentTarget)
			Shape.CreateArrow(owner.GetOrigin(), m_CurrentTarget.m_Ent.GetOrigin(), 0.1, COLOR_GREEN, ShapeFlags.ONCE);
		
		m_fSearchTimer -= timeSlice;	
		if (m_fSearchTimer > 0)
			return;
		
		m_fSearchTimer = m_fMaxSearchTime;
		
		if (!m_CurrentTarget || !CheckTarget())
			m_CurrentTarget = FindTarget();
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_MainTurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));
		m_AimingComp = BON_AutoTurretAimingComponent.Cast(owner.FindComponent(BON_AutoTurretAimingComponent));
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionComp.GetAffiliatedFaction();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
