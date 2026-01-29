[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class BON_GuidedProjectileClass : ProjectileClass
{
}

class BON_GuidedProjectile : Projectile
{
	[Attribute(defvalue: "10", desc: "Guidance Strength, how fast to turn towards target", params: "0 inf 0.01")]
	float m_fGuidanceStrength;

	[Attribute("1", UIWidgets.Auto, "Max turn rate limit (rad/s)")]
	protected float m_fMaxTurnRate;

	[Attribute("50", UIWidgets.Slider, "What % chance does a single countermeasure fire have? (def: 50)", "0 100 1")]
	protected int m_iCountermeasureSuccessChance;

	[Attribute("1", UIWidgets.Auto, "Max turn rate limit (rad/s)")]
	protected int m_iMaxTurnRate;

	int m_fSelfDestructTime = 1;

	protected EventHandlerManagerComponent m_EventHandlerManager;
	int m_iSelfDestructTime;
	IEntity m_TrackedTarget;
	vector m_vAimOffset;
	vector m_vLastDirToTarget;
	BON_TurretFireMode m_eFireMode;
	MissileMoveComponent m_MissileMove;
	float m_fSpeed;

	//------------------------------------------------------------------------------------------------
	//No need to get trigger comp on init, called once
	void Trigger()
	{
		#ifdef WCS_DEFINES_ARMAMENTS
		m_EventHandlerManager.RemoveScriptHandler(WCS_Armament_ChaffDispenserComponent.CHAFF_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);
		m_EventHandlerManager.RemoveScriptHandler(WCS_Armament_FlareDispenserComponent.FLARE_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);
		#endif

		BaseTriggerComponent triggerComp = BaseTriggerComponent.Cast(FindComponent(BaseTriggerComponent));
		triggerComp.OnUserTrigger(this);
	}

	//------------------------------------------------------------------------------------------------
	void SteerToTarget(float timeSlice)
	{
		if (System.GetUnixTime() > m_iSelfDestructTime)
		{
			Trigger();
		}

		vector targetPos = m_TrackedTarget.GetOrigin();
		vector targetVel = m_TrackedTarget.GetPhysics().GetVelocity();
		float targetDistance = vector.Distance(m_TrackedTarget.GetOrigin(), GetOrigin());

		if (targetDistance <= 1)
			Trigger();

		//Add lead
		if (m_eFireMode == BON_TurretFireMode.Intercept)
		{
			float timeToTarget = targetDistance / m_fSpeed;
			targetPos += targetVel * timeToTarget;
		}

		vector dirToTarget = targetPos - GetOrigin();
		dirToTarget.Normalize();
		vector localFwd = GetTransformAxis(2).Normalized();
		vector axis = SCR_Math3D.Cross(localFwd, dirToTarget);

		float dot = vector.Dot(localFwd, dirToTarget);
		dot = Math.Clamp(dot, -1.0, 1.0);
		float angleRad = Math.Acos(dot);

		float turnRate = Math.Min(angleRad / timeSlice, m_fMaxTurnRate);
		vector angularVel = axis * turnRate;

		m_MissileMove.SetVelocity(localFwd * m_fSpeed);
		m_MissileMove.SetAngularVelocity(angularVel);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_TrackedTarget)
			SteerToTarget(timeSlice);

	}
	
	#ifdef WCS_DEFINES_ARMAMENTS
	//Official WCS compatibility. Thanks to Cyborgmatt :)
	//------------------------------------------------------------------------------------------------
	//! % chance to succeed
	void OnCounterMeasuresFired()
	{
		if (Math.RandomInt(0, 100) <= m_iCountermeasureSuccessChance)
			GetGame().GetCallqueue().CallLater(SearchCounterMeasures, 100);
	}

	//------------------------------------------------------------------------------------------------
	void SearchCounterMeasures()
	{
		GetGame().GetWorld().QueryEntitiesBySphere(m_TrackedTarget.GetOrigin(), 10, QueryEntities, FilterEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected bool FilterEntity(IEntity ent)
	{
		return Projectile.Cast(ent) != null;
	}
	
	//------------------------------------------------------------------------------------------------
	bool QueryEntities(IEntity ent)
	{
		WCS_Armament_ChaffComponent chaff = WCS_Armament_ChaffComponent.Cast(ent.FindComponent(WCS_Armament_ChaffComponent));
		WCS_Armament_FlareComponent flare = WCS_Armament_FlareComponent.Cast(ent.FindComponent(WCS_Armament_FlareComponent));

		if (chaff || flare)
		{
			m_TrackedTarget = ent;
			return false; //Stop search
		}

		return true; //Continue search
	}
	#endif

	//------------------------------------------------------------------------------------------------
	void Launch(IEntity target)
	{
		if (!target)
			return;

		m_MissileMove = MissileMoveComponent.Cast(FindComponent(MissileMoveComponent));
		m_MissileMove.Launch(GetTransformAxis(2), vector.Zero, 1, this, null, null, null, null);

		m_TrackedTarget = target;

		float targetDistance = vector.Distance(m_TrackedTarget.GetOrigin(), GetOrigin());
		float timeToTarget = targetDistance / m_fSpeed;
		m_iSelfDestructTime = System.GetUnixTime() + (int)timeToTarget + m_fSelfDestructTime;

		GetPhysics().SetActive(ActiveState.ACTIVE);

		m_vLastDirToTarget = target.GetOrigin() - GetOrigin();
		m_vLastDirToTarget.Normalize();

		SetEventMask(EntityEvent.FRAME);

		//Official WCS compatibility. Thanks to Cyborgmatt :)
		#ifdef WCS_DEFINES_ARMAMENTS		
		m_EventHandlerManager = EventHandlerManagerComponent.Cast(target.FindComponent(EventHandlerManagerComponent));
		m_EventHandlerManager.RegisterScriptHandler(WCS_Armament_ChaffDispenserComponent.CHAFF_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);
		m_EventHandlerManager.RegisterScriptHandler(WCS_Armament_FlareDispenserComponent.FLARE_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);		
		#endif		
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_Launch(RplId targetID, BON_TurretFireMode fireMode, float speed)
	{
		m_fSpeed = speed;
		m_eFireMode = fireMode;

		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(targetID));
		IEntity target = rplComponent.GetEntity();

		Launch(target);
	}

	//------------------------------------------------------------------------------------------------
	void DelayLaunch(IEntity target, BON_TurretFireMode fireMode, float speed)
	{
		//Broadcast all data for clients to also steer to target (steering is not replicated)
		RplComponent targetRplComp = RplComponent.Cast(target.FindComponent(RplComponent));
		Rpc(RpcDo_Launch, targetRplComp.Id(), fireMode, speed);

		Launch(target);
	}

	//------------------------------------------------------------------------------------------------
	//! Called from Server
	void SetTargetAndLaunch(IEntity target, BON_TurretFireMode fireMode, float speed)
	{
		if (!Replication.IsServer() || !target)
			return;

		m_fSpeed = speed;
		m_eFireMode = fireMode;

		GetGame().GetCallqueue().CallLater(DelayLaunch, 1, false, target, fireMode, speed);
	}

	//------------------------------------------------------------------------------------------------
	void ~BON_GuidedProjectile()
	{
		#ifdef WCS_DEFINES_ARMAMENTS
		if (!m_EventHandlerManager)
			return;
		m_EventHandlerManager.RemoveScriptHandler(WCS_Armament_ChaffDispenserComponent.CHAFF_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);
		m_EventHandlerManager.RemoveScriptHandler(WCS_Armament_FlareDispenserComponent.FLARE_COUNT_CHANGED_EVENT, this, OnCounterMeasuresFired);
		#endif
	}
}
