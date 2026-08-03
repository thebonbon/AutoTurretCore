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

	[Attribute("100", UIWidgets.Auto, desc: "", category: "Setup"), RplProp()]
	float m_fRotationSpeed;

	[Attribute("10", UIWidgets.Auto, desc: "", category: "Setup"), RplProp()]
	float m_fIdleRotationSpeed;

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

	TNodeId m_iBarrelBoneIndex;
	TNodeId m_iBodyBoneIndex;
	protected BON_AutoTurretComponent m_TurretComp;
	protected ref BON_AutoTurretTarget m_Target;
	protected vector m_vCurrentAngles;
	protected vector m_vTargetAngles;
	protected int m_iIdleDirection;

	[RplProp()]
	bool m_bOnTarget;

	// ── Rotation replication (fixes clients never seeing turret rotation) ────
	// Server computes m_vCurrentAngles and drives the local AnimGraph signals
	// directly, but clients never ran EOnInit's signal setup (server-only
	// guard below) and never received the angle updates. RplProp replicates
	// the computed angles to every client, which then writes them into its
	// own local signals via ATC_OnRotationSynced().
	[RplProp(onRplName: "ATC_OnRotationSynced")]
	protected float m_fATC_SyncedBody;

	[RplProp(onRplName: "ATC_OnRotationSynced")]
	protected float m_fATC_SyncedBarrel;

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
		bool inHorizontal = Math.IsInRange(angles[0], m_vLimitHorizontal[0], m_vLimitHorizontal[1]);
		bool inVertical = Math.IsInRange(angles[1], m_vLimitVertical[0], m_vLimitVertical[1]);

		return (inHorizontal && inVertical);
	}

	//------------------------------------------------------------------------------------------------
	bool IsWithinLimitsPos(BON_AutoTurretTarget target)
	{
		vector barrelMat[4];
		vector ownerMat[4];
		GetBoneWorldTransform(m_iBarrelBoneIndex, barrelMat);
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
		GetBoneWorldTransform(m_iBarrelBoneIndex, barrelMat);
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
	bool IsOnTarget()
	{
		return m_bOnTarget;
	}

	//------------------------------------------------------------------------------------------------
	void HandleRotatingToTarget(float timeSlice)
	{
		vector targetAngles = vector.Zero;

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
		GetBoneWorldTransform(m_iBarrelBoneIndex, barrelMat);
		GetOwner().GetWorldTransform(ownerMat);

		targetAngles = SCR_Math3D.ComputeTargetAngles(ownerMat, barrelMat[3], aimPoint);

		if (!IsWithinLimitsAngle(targetAngles))
			targetAngles = vector.Zero;

		RotateTo(targetAngles, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void SetOnTarget(bool onTarget)
	{
		m_bOnTarget = onTarget;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateOnTarget(float yawDelta, float pitchDelta)
	{
		bool inYawRange = Math.AbsFloat(yawDelta) < m_fAngleTolerance;
		bool inPitchRange = Math.AbsFloat(pitchDelta) < m_fAngleTolerance;

		bool onTarget = inYawRange && inPitchRange;

		if (m_bOnTarget == onTarget)
			return;

		SetOnTarget(onTarget);
	}

	//------------------------------------------------------------------------------------------------
	//! Rotate to desired angles within limits
	void RotateTo(vector targetAngles, float timeSlice)
	{
		m_vTargetAngles = targetAngles;

		float maxStep = m_fRotationSpeed * timeSlice;

		float yawDelta = SCR_Math3D.WrapAngleDiffDeg(m_vTargetAngles[0] - m_vCurrentAngles[0]);
		float pitchDelta = SCR_Math3D.WrapAngleDiffDeg(m_vTargetAngles[1] - m_vCurrentAngles[1]);

		if (targetAngles != vector.Zero)
			UpdateOnTarget(yawDelta, pitchDelta);

		yawDelta = Math.Clamp(yawDelta, -maxStep, maxStep);
		pitchDelta = Math.Clamp(pitchDelta, -maxStep, maxStep);

		m_vCurrentAngles[0] = SCR_Math3D.WrapAngleDiffDeg(m_vCurrentAngles[0] + yawDelta);
		m_vCurrentAngles[1] = SCR_Math3D.WrapAngleDiffDeg(m_vCurrentAngles[1] + pitchDelta);

		m_SignalsManager.SetSignalValue(m_iSignalBody, -m_vCurrentAngles[0]);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_vCurrentAngles[1]);

		SyncRotation();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: replicate the current angles to clients so their local
	//! signals (and therefore the AnimGraph) actually move.
	protected void SyncRotation()
	{
		if (!Replication.IsServer())
			return;

		m_fATC_SyncedBody = -m_vCurrentAngles[0];
		m_fATC_SyncedBarrel = m_vCurrentAngles[1];
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Client: fires automatically whenever the replicated angles change
	void ATC_OnRotationSynced()
	{
		if (!m_SignalsManager)
			return;

		if (m_iSignalBody != -1)
			m_SignalsManager.SetSignalValue(m_iSignalBody, m_fATC_SyncedBody);

		if (m_iSignalBarrel != -1)
			m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fATC_SyncedBarrel);
	}

	//------------------------------------------------------------------------------------------------
	//! Gets bone MODEL space (from scene root) and returns it as WORLD space
	void GetBoneWorldTransform(TNodeId boneIndex, out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();

		vector localBoneMat[4];
		ownerAnim.GetBoneMatrix(boneIndex, localBoneMat);

		vector ownerMat[4];
		GetOwner().GetWorldTransform(ownerMat);

		Math3D.MatrixMultiply4(ownerMat, localBoneMat, mat);
	}

	//------------------------------------------------------------------------------------------------
	void HandleIdleSearching(float timeSlice)
	{
		float maxYaw = m_vLimitHorizontal[1];
		float minYaw = m_vLimitHorizontal[0];

		if (m_iIdleDirection == 0)
			m_iIdleDirection = 1;

		float yaw = m_vCurrentAngles[0];
		yaw = yaw + (m_fIdleRotationSpeed * timeSlice * m_iIdleDirection);

		if (yaw >= maxYaw)
		{
			yaw = maxYaw;
			m_iIdleDirection = -1;
		}
		else if (yaw <= minYaw)
		{
			yaw = minYaw;
			m_iIdleDirection = 1;
		}

		float pitch = m_vCurrentAngles[1];
		float pitchDelta = SCR_Math3D.WrapAngleDiffDeg(0 - pitch);
		float pitchStep = m_fIdleRotationSpeed * timeSlice;
		pitchDelta = Math.Clamp(pitchDelta, -pitchStep, pitchStep);
		pitch += pitchDelta;

		m_vCurrentAngles[0] = SCR_Math3D.WrapAngleDiffDeg(yaw);
		m_vCurrentAngles[1] = SCR_Math3D.WrapAngleDiffDeg(pitch);

		m_SignalsManager.SetSignalValue(m_iSignalBody, -m_vCurrentAngles[0]);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_vCurrentAngles[1]);

		SyncRotation();
	}

	//------------------------------------------------------------------------------------------------
	//! Server only
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		m_Target = target;

		if (m_Target)
			HandleRotatingToTarget(timeSlice);
		else if (m_fIdleRotationSpeed != 0)
			HandleIdleSearching(timeSlice);
		else
			RotateTo(vector.Zero, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// Signals manager + signal indices are needed on EVERY machine now,
		// not just the server: clients drive their own local signals from
		// the replicated angles in ATC_OnRotationSynced(), so they need a
		// valid m_SignalsManager / m_iSignalBody / m_iSignalBarrel too.
		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));

		if (m_SignalsManager)
		{
			if (Replication.IsServer())
			{
				// Only the server registers/owns the MP signal.
				m_iSignalBody = m_SignalsManager.AddOrFindMPSignal("BodyRotation", 0.1, 1);
				m_iSignalBarrel = m_SignalsManager.AddOrFindMPSignal("BarrelRotation", 0.1, 1);
			}
			else
			{
				// Clients just look up the signal that AutoVariablesBind /
				// the server already registered on the prefab.
				m_iSignalBody = m_SignalsManager.FindSignal("BodyRotation");
				m_iSignalBarrel = m_SignalsManager.FindSignal("BarrelRotation");
			}
		}

		if (!Replication.IsServer())
			return;

		m_TurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));

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

		vector bodyMat[4];
		vector barrelMat[4];
		GetBoneWorldTransform(m_iBodyBoneIndex, bodyMat);
		GetBoneWorldTransform(m_iBarrelBoneIndex, barrelMat);

		CreateCircleSlice(barrelMat[3], -owner.GetTransformAxis(0).Normalized(), owner.GetTransformAxis(2).Normalized(),
			m_vLimitVertical[0], m_vLimitVertical[1], 5, Color.RED, 32, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP |ShapeFlags.ONCE);

		CreateCircleSlice(bodyMat[3], owner.GetTransformAxis(1).Normalized(), owner.GetTransformAxis(2).Normalized(),
			m_vLimitHorizontal[0], m_vLimitHorizontal[1], 5, Color.BLUE, 32, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP |ShapeFlags.ONCE);

	}
}