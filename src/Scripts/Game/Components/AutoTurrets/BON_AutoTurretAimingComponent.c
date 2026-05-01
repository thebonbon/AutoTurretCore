
enum BON_TurretAimState
{
	IDLE,
	ROTATING_TO_TARGET
}

[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretAimingComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretAimingComponent : ScriptComponent
{
	[Attribute("-180 180 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z. Keep in range -180,180", category: "Setup")]
	protected vector m_vLimitHorizontal;

	[Attribute("-25 85 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z, Keep in range -180,180", category: "Setup")]
	protected vector m_vLimitVertical;

	[Attribute("1", UIWidgets.Auto, desc: "", category: "Setup"), RplProp()]
	float m_fRotationSpeed;

	[Attribute("w_body", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBodyBone;

	[Attribute("w_barrel", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBarrelBone;

	[Attribute("false", UIWidgets.CheckBox, "Show Aiming debug?", category: "Debug")]
	protected bool m_bDebug;

	[Attribute("2", UIWidgets.Auto, desc: "Angle Tolerance (degrees)", category: "Setup"), RplProp()]
	float m_fAngleTolerance;

	protected SignalsManagerComponent m_SignalsManager;
	protected int m_iSignalBody;
	protected int m_iSignalBarrel;

	protected TNodeId m_iBarrelBoneIndex;
	protected TNodeId m_iBodyBoneIndex;
	protected BON_AutoTurretComponent m_TurretComp;
	protected ref BON_AutoTurretTarget m_Target;
	protected vector m_vCurrentAngles;
	protected vector m_vTargetAngles;

	//------------------------------------------------------------------------------------------------
	void SetAngleTolerance(float tolerance)
	{
		m_fAngleTolerance = tolerance;
		Replication.BumpMe();
	}
	//------------------------------------------------------------------------------------------------
	void SetRotationSpeed(float speed)
	{
		m_fRotationSpeed = speed;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	bool IsWithinLimitsAngle(vector angles)
	{
		float yaw = angles[0];
		float pitch = angles[1];

		bool inHorizontal = Math.IsInRange(yaw, m_vLimitHorizontal[0], m_vLimitHorizontal[1]);
		bool inVertical = Math.IsInRange(pitch, m_vLimitVertical[0], m_vLimitVertical[1]);

		return (inHorizontal && inVertical);
	}

	//------------------------------------------------------------------------------------------------
	bool IsWithinLimitsPos(BON_AutoTurretTarget target)
	{
		vector barrelMat[4];
		vector ownerMat[4];
		GetBarrelWorldTransform(barrelMat);
		GetOwner().GetWorldTransform(ownerMat);

		vector angles = SCR_Math3D.ComputeTargetAngles(ownerMat, barrelMat[3], target.GetAimPoint());

		return IsWithinLimitsAngle(angles);
	}

	//------------------------------------------------------------------------------------------------
	//! Predicts the position with given speed to target pos
	vector ComputeLeadSimple()
	{
		if (!m_Target.IsValid())
			return vector.Zero;

		vector targetVelocity = m_Target.m_Ent.GetPhysics().GetVelocity();
		float targetDistance = vector.Distance(m_Target.m_Ent.GetOrigin(), GetOwner().GetOrigin());
		float timeToTarget = targetDistance / m_TurretComp.m_fProjectileSpeed;

		return targetVelocity * timeToTarget;
	}

	//------------------------------------------------------------------------------------------------
	//! Lead offset (velocity, time) needed to hit target
	vector ComputeLead()
	{
		if (!m_Target.IsValid())
			return vector.Zero;

		vector predictedLeadingOffset;

		vector barrelMat[4];
		GetBarrelWorldTransform(barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		Resource projectileResource = Resource.Load(m_TurretComp.m_Projectile);
		IEntitySource projectileSource = SCR_BaseContainerTools.FindEntitySource(projectileResource);

		float timeToTarget;
		vector muzzleMat[4];
		vector effectMat[4];
		m_TurretComp.GetMuzzleTransform(muzzleMat, effectMat);

		float targetDistance = vector.Distance(muzzleMat[3], m_Target.GetAimPoint());
		float heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);

		Physics targetRB = m_Target.m_Ent.GetPhysics();

		if (targetRB && targetRB.IsActive())
		{
			//Add Leading
			predictedLeadingOffset = targetRB.GetVelocity() * timeToTarget;
			targetDistance = vector.Distance(muzzleMat[3], m_Target.GetAimPoint() + predictedLeadingOffset);
			heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);
		}

		//Add Ballistics
		predictedLeadingOffset[1] = heightOffset;

		return predictedLeadingOffset;
	}

	//------------------------------------------------------------------------------------------------
	bool CanFire()
	{
		return (m_Target && IsOnTarget());
	}

	//------------------------------------------------------------------------------------------------
	bool IsOnTarget()
	{
		if (m_vTargetAngles == Vector(-1, -1, -1))
			return false;

		vector current = SCR_Math3D.FixEulerVector180(m_vCurrentAngles);
		vector target = SCR_Math3D.FixEulerVector180(m_vTargetAngles);

		return Math.AbsFloat(current[0] - target[0]) < m_fAngleTolerance
			&& Math.AbsFloat(current[1] - target[1]) < m_fAngleTolerance;
	}

	//------------------------------------------------------------------------------------------------
	void HandleRotatingToTarget(float timeSlice)
	{
		vector targetAngles = vector.Zero;

		if (m_Target)
		{
			vector aimPoint = m_Target.GetAimPoint();

			if (m_TurretComp.m_eFireMode == BON_TurretFireMode.Intercept)
			{
				if (m_TurretComp.m_bIsMissile)
					aimPoint += ComputeLeadSimple();
				else
					aimPoint += ComputeLead();
			}

			vector barrelMat[4];
			vector ownerMat[4];
			GetBarrelWorldTransform(barrelMat);
			GetOwner().GetWorldTransform(ownerMat);

			targetAngles = SCR_Math3D.ComputeTargetAngles(ownerMat, barrelMat[3], aimPoint);
		}

		RotateTo(targetAngles, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	//! Rotate to desired angles within limits
	void RotateTo(vector targetAngles, float timeSlice)
	{
		if (!IsWithinLimitsAngle(targetAngles))
		{
			m_vTargetAngles = Vector(-1, -1, -1);
			return;
		}

		m_vTargetAngles = targetAngles;

		float maxStep = m_fRotationSpeed * timeSlice;

		float yawDelta = SCR_Math3D.WrapAngleDiffDeg(m_vTargetAngles[0] - m_vCurrentAngles[0]);
		float pitchDelta = SCR_Math3D.WrapAngleDiffDeg(m_vTargetAngles[1] - m_vCurrentAngles[1]);

		yawDelta = Math.Clamp(yawDelta, -maxStep, maxStep);
		pitchDelta = Math.Clamp(pitchDelta, -maxStep, maxStep);

		m_vCurrentAngles[0] = SCR_Math3D.WrapAngleDiffDeg(m_vCurrentAngles[0] + yawDelta);
		m_vCurrentAngles[1] = SCR_Math3D.WrapAngleDiffDeg(m_vCurrentAngles[1] + pitchDelta);

		m_SignalsManager.SetSignalValue(m_iSignalBody, -m_vCurrentAngles[0]);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_vCurrentAngles[1]);
	}

	//------------------------------------------------------------------------------------------------
	void GetBodyWorldTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		vector localBoneMat[4];
		ownerAnim.GetBoneMatrix(m_iBodyBoneIndex, localBoneMat); // MODEL space to Scene root

		//Convert to WORLD space
		vector ownerMat[4];
		GetOwner().GetWorldTransform(ownerMat);
		Math3D.MatrixMultiply4(ownerMat, localBoneMat, mat);
	}

	//------------------------------------------------------------------------------------------------
	void GetBarrelWorldTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		vector localBoneMat[4];
		ownerAnim.GetBoneMatrix(m_iBarrelBoneIndex, localBoneMat); // MODEL space to Scene root

		//Convert to WORLD space
		vector ownerMat[4];
		GetOwner().GetWorldTransform(ownerMat);
		Math3D.MatrixMultiply4(ownerMat, localBoneMat, mat);
	}

	//------------------------------------------------------------------------------------------------
	//! Servers
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		// Prevent firing on stale idle angles
		if (target != m_Target)
			m_vTargetAngles = Vector(-1, -1, -1);

		m_Target = target;

		if (m_Target)
			HandleRotatingToTarget(timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;

		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_TurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));

		m_iSignalBody = m_SignalsManager.AddOrFindMPSignal("BodyRotation", 0.1, 1);
		m_iSignalBarrel = m_SignalsManager.AddOrFindMPSignal("BarrelRotation", 0.1, 1);

		Animation anim = GetOwner().GetAnimation();
		if (anim)
		{
			m_iBarrelBoneIndex = anim.GetBoneIndex(m_sBarrelBone);
			m_iBodyBoneIndex = anim.GetBoneIndex(m_sBodyBone);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bDebug)
			return;

		Animation ownerAnim = owner.GetAnimation();
		vector boneMat[4];
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBodyBone), boneMat);
		vector barrelMat[4];
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBarrelBone), barrelMat);

		boneMat[3] = owner.CoordToParent(boneMat[3]);
		barrelMat[3] = owner.CoordToParent(barrelMat[3]);

		CreateCircleSlice(barrelMat[3], -owner.GetTransformAxis(0).Normalized(), owner.GetTransformAxis(2).Normalized(),
			m_vLimitVertical[0], m_vLimitVertical[1], 5, Color.RED, 32, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP |ShapeFlags.ONCE);

		CreateCircleSlice(boneMat[3], owner.GetTransformAxis(1).Normalized(), owner.GetTransformAxis(2).Normalized(),
			m_vLimitHorizontal[0], m_vLimitHorizontal[1], 5, Color.BLUE, 32, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP |ShapeFlags.ONCE);

	}
}
