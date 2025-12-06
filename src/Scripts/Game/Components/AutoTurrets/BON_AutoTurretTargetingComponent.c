[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetingComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetingComponent : ScriptComponent
{
	
	[Attribute(uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	BON_TurretTargetFilterFlags m_eTargetFlags;
	
	[Attribute("500", UIWidgets.Auto, "Attack range (m)", category: "Aiming")]
	int m_iSearchRadius;
	
	//TODO: ATTRIVUTE THIS
	protected float m_fMaxSearchTime = 0.1;
	
	protected float m_fSearchTimer = 0;
	protected BON_AutoTurretAimingComponent m_aimingComp;
	protected Faction m_Faction;
	
	protected ref BON_AutoTurretTarget m_CurrentTarget;
	BON_AutoTurretComponent m_MainTurretComp;
	
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

		/*if (m_bDebug)
		{
			vector position = (param.End - param.Start) * traceDistance + param.Start;
			m_LoSDebug = Shape.CreateArrow(muzzleMat[3], position, 0.1, COLOR_GREEN, ShapeFlags.NOZBUFFER);
		}*/

		//Full distance or trace hit the target entity
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
			//if (m_aimingComp.IsWithinLimits(target.GetAimPoint()) && LineOfSightTo(target))
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
		m_aimingComp = BON_AutoTurretAimingComponent.Cast(owner.FindComponent(BON_AutoTurretAimingComponent));
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionComp.GetAffiliatedFaction();
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
