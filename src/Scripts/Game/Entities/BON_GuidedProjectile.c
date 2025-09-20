[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class BON_GuidedProjectileClass : ProjectileClass
{
}

class BON_GuidedProjectile : Projectile
{
	[Attribute()]
	float m_fMoveSpeed;

	[Attribute()]
	float m_fGuidanceStrength;
	
	[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iTrackedTargetId;

	IEntity m_TrackedTarget;
	vector m_vAimOffset;
	vector m_vLastDirToTarget;
	BON_TurretFireMode m_eFireMode;
	
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
		vector missilePos = GetOrigin();
		
		vector targetPos = m_TrackedTarget.GetOrigin();
		vector targetVel = m_TrackedTarget.GetPhysics().GetVelocity();
		float targetDistance = vector.Distance(m_TrackedTarget.GetOrigin(), GetOrigin());
		
		if (targetDistance < 1)
		{
			BaseTriggerComponent triggerComp = BaseTriggerComponent.Cast(FindComponent(BaseTriggerComponent));
			triggerComp.OnUserTrigger(this);
		}
		
		float timeToTarget = targetDistance / m_fMoveSpeed;
		vector newTargetPos;
		if (m_eFireMode == BON_TurretFireMode.Intercept)
		{
			newTargetPos = targetPos + targetVel * timeToTarget;
		}
		else
			newTargetPos = targetPos;
		
		//Shape.CreateSphere(COLOR_GREEN, ShapeFlags.ONCE | ShapeFlags.NOZWRITE, newTargetPos, 0.5);
		
		
		vector dirToTarget = newTargetPos - missilePos;
		dirToTarget.Normalize();
		
		vector localFwd = GetTransformAxis(2).Normalized();
		
		vector axis = Cross(localFwd, dirToTarget);
		float dot = vector.Dot(localFwd, dirToTarget);
		dot = Math.Clamp(dot, -1.0, 1.0);
		float angleRad = Math.Acos(dot);
		
		vector angularVel = axis * angleRad * 100;
		
		GetPhysics().SetVelocity(localFwd * m_fMoveSpeed);
		GetPhysics().SetAngularVelocity(angularVel);

	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_TrackedTarget)
			SteerToTarget(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	vector Cross(vector a, vector b)
	{
		return Vector(
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0]
		);
	}

	//------------------------------------------------------------------------------------------------
	void SetTargetAndLaunch(IEntity target, BON_TurretFireMode fireMode)
	{
		if (!target)
			return;
		m_eFireMode = fireMode;
		
		RplComponent targetRplComp = RplComponent.Cast(target.FindComponent(RplComponent));
		m_iTrackedTargetId = targetRplComp.Id();
		Replication.BumpMe();

		MissileMoveComponent missileMove = MissileMoveComponent.Cast(FindComponent(MissileMoveComponent));
		missileMove.Launch(vector.Zero, vector.Zero, 0, this, null, null, null, null);

		Launch(target);
	}

	//------------------------------------------------------------------------------------------------
	void Launch(IEntity target)
	{
		m_TrackedTarget = target;
		GetPhysics().SetActive(ActiveState.ACTIVE);

		PerceivableComponent targetPerceivableComp = PerceivableComponent.Cast(target.FindComponent(PerceivableComponent));

		if (targetPerceivableComp)
		{
			array<vector> aimPoints();
			targetPerceivableComp.GetAimpoints(aimPoints);
			m_vAimOffset = target.CoordToLocal(aimPoints[0]);
		}
		else
			m_vAimOffset = Vector(0, 2, 0);

		m_vLastDirToTarget = target.CoordToParent(m_vAimOffset) - GetOrigin();
		m_vLastDirToTarget.Normalize();

		SetEventMask(EntityEvent.FRAME);
	}
}
