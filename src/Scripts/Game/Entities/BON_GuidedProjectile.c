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
	Physics m_Rb;
	vector m_vAimOffset;
	vector m_vLastDirToTarget;
	
	
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
		vector localFwd = GetTransformAxis(2).Normalized();
		vector dirToTarget = m_TrackedTarget.CoordToParent(m_vAimOffset) - GetOrigin();
		dirToTarget.Normalize();

		vector dirChangeRate = (dirToTarget - m_vLastDirToTarget) / timeSlice;
		vector angularVelocity = Cross(dirToTarget, dirChangeRate);

		m_Rb.SetVelocity(localFwd * m_fMoveSpeed);
		m_Rb.SetAngularVelocity(angularVelocity * m_fGuidanceStrength);

		m_vLastDirToTarget = dirToTarget;
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
	void SetTargetAndLaunch(IEntity target)
	{
		if (!target)
			return;

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
		m_Rb.SetActive(ActiveState.ACTIVE);

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

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		if (!GetGame().InPlayMode())
			return;

		m_Rb = GetPhysics();
	}

	//------------------------------------------------------------------------------------------------
	void BON_GuidedProjectile(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
	}
}
