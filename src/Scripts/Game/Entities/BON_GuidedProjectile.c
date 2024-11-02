[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class BON_GuidedProjectileClass : ProjectileClass
{
}

class BON_GuidedProjectile : Projectile
{
	[Attribute()]
	float m_fTrackDelay;
	
	[Attribute()]
	float m_fMoveSpeed;
	
	[Attribute()]
	float m_fRotSpeed;
	
	IEntity m_TrackedTarget;
	Physics m_Rb;
	vector m_vAimOffset;

	//------------------------------------------------------------------------------------------------
	void SteerToTarget(float timeSlice)
	{
		vector localFwd = GetTransformAxis(2).Normalized();
		vector dirToTarget;
		if (m_vAimOffset)
			dirToTarget = m_vAimOffset - GetOrigin();
		else
			dirToTarget = m_TrackedTarget.GetOrigin() + m_TrackedTarget.GetTransformAxis(1).Normalized() * 2 - GetOrigin(); //Default up offset to not hit ground pivot
		
		dirToTarget.Normalize();
		
		vector rotationAxis = Cross(localFwd, dirToTarget).Normalized();
		float dot = vector.Dot(localFwd, dirToTarget);
		float angleDifference = Math.Acos(Math.Clamp(dot, -1.0, 1.0));

		float losRate = angleDifference / timeSlice;
		
		m_Rb.SetVelocity(localFwd * m_fMoveSpeed);
		m_Rb.SetAngularVelocity(rotationAxis * losRate * m_fRotSpeed);
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
	void Launch(IEntity target)
	{
		m_TrackedTarget = target;
		m_Rb.SetActive(ActiveState.ACTIVE);
		
		MissileMoveComponent missileMove = MissileMoveComponent.Cast(FindComponent(MissileMoveComponent));
		missileMove.Launch(vector.Zero, vector.Zero, 0, this, null, null, null, null);
		
		PerceivableComponent targetPerceivableComp = PerceivableComponent.Cast(target.FindComponent(PerceivableComponent));
		if (targetPerceivableComp)
		{
			array<vector> aimPoints();
			targetPerceivableComp.GetAimpoints(aimPoints);
			m_vAimOffset = aimPoints[0];
		}
		
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

	//------------------------------------------------------------------------------------------------
	void ~BON_GuidedProjectile()
	{
	}
}