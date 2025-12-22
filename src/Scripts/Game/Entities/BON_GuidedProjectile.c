[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class BON_GuidedProjectileClass : ProjectileClass
{
}

class BON_GuidedProjectile : Projectile
{
	[Attribute(defvalue: "10", desc: "Guidance Strength, how fast to turn towards target", params: "0 inf 0.01")]
	float m_fGuidanceStrength;

	[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iTrackedTargetId;

	IEntity m_TrackedTarget;
	vector m_vAimOffset;
	vector m_vLastDirToTarget;
	BON_TurretFireMode m_eFireMode;
	MissileMoveComponent m_MissileMove;
	float m_fSpeed;

	//------------------------------------------------------------------------------------------------
	void OnTargetChanged()
	{
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(m_iTrackedTargetId));
		if (rplComponent)
		{
			Launch(rplComponent.GetEntity());
		}
	}

	//------------------------------------------------------------------------------------------------
	void SteerToTarget(float timeSlice)
	{
		vector targetPos = m_TrackedTarget.GetOrigin();
		vector targetVel = m_TrackedTarget.GetPhysics().GetVelocity();
		float targetDistance = vector.Distance(m_TrackedTarget.GetOrigin(), GetOrigin());

		//TODO: Check for trigger on target from turret?
		if (targetDistance < 5)
		{
			BaseTriggerComponent triggerComp = BaseTriggerComponent.Cast(FindComponent(BaseTriggerComponent));
			triggerComp.OnUserTrigger(this);
			return;
		}

		//Add lead
		if (m_eFireMode == BON_TurretFireMode.Intercept)
		{
			float timeToTarget = targetDistance / m_fSpeed;
			targetPos += targetVel * timeToTarget;
		}

		Shape.CreateSphere(Color.YELLOW, ShapeFlags.ONCE, targetPos, 20);
		vector dirToTarget = targetPos - GetOrigin();
		dirToTarget.Normalize();

		vector localFwd = GetTransformAxis(2).Normalized();

		vector axis = SCR_Math3D.Cross(localFwd, dirToTarget);
		float dot = vector.Dot(localFwd, dirToTarget);
		dot = Math.Clamp(dot, -1.0, 1.0);
		float angleRad = Math.Acos(dot);

		vector angularVel = axis * angleRad * m_fGuidanceStrength;

		m_MissileMove.SetVelocity(localFwd * m_fSpeed);
		m_MissileMove.SetAngularVelocity(angularVel);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_TrackedTarget)
			SteerToTarget(timeSlice);
	}


	//------------------------------------------------------------------------------------------------
	void Launch(IEntity target)
	{
		m_TrackedTarget = target;
		GetPhysics().SetActive(ActiveState.ACTIVE);

		m_vLastDirToTarget = target.GetOrigin() - GetOrigin();
		m_vLastDirToTarget.Normalize();

		SetEventMask(EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	void SetTargetAndLaunch(IEntity target, BON_TurretFireMode fireMode, float speed)
	{
		if (!target)
			return;

		m_fSpeed = speed;
		m_eFireMode = fireMode;

		RplComponent targetRplComp = RplComponent.Cast(target.FindComponent(RplComponent));
		m_iTrackedTargetId = targetRplComp.Id();
		Replication.BumpMe();

		m_MissileMove = MissileMoveComponent.Cast(FindComponent(MissileMoveComponent));
		m_MissileMove.Launch(GetTransformAxis(2), vector.Zero, 1, this, null, null, null, null);

		Launch(target);
	}
}
