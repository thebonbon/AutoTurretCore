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
	[Attribute("-180 180 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitHorizontal;

	[Attribute("-25 85 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
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
	float m_fAngleTolerance; // degrees

	protected SignalsManagerComponent m_SignalsManager;
	protected int m_iSignalBody;
	protected int m_iSignalBarrel;

	protected TNodeId m_iBarrelBoneIndex;
	protected BON_AutoTurretComponent m_TurretComp
	protected ref BON_AutoTurretTarget m_Target;
	protected BON_TurretAimState m_eAimState = BON_TurretAimState.IDLE;
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
		vector mat[4];
		GetOwner().GetTransform(mat);
		vector angles = SCR_Math3D.GetLocalAngles(mat, target.GetAimPoint());

		return IsWithinLimitsAngle(angles);
	}

	//------------------------------------------------------------------------------------------------
	//! Predicts the position with given speed to target pos
	vector ComputeLeadSimple()
	{
		vector targetVelocity = m_Target.m_Ent.GetPhysics().GetVelocity();
		float targetDistance = vector.Distance(m_Target.m_Ent.GetOrigin(), GetOwner().GetOrigin());
		float timeToTarget = targetDistance / m_TurretComp.m_fProjectileSpeed;

		return targetVelocity * timeToTarget;
	}

	//------------------------------------------------------------------------------------------------
	//! Lead offset (velocity, time) needed to hit target
	vector ComputeLead()
	{
		if (!m_Target.m_Ent)
			return vector.Zero;

		vector predictedLeadingOffset;

		vector barrelMat[4];
		GetBarrelTransform(barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		Resource projectileResource = Resource.Load(m_TurretComp.m_Projectile);
		IEntitySource projectileSource = SCR_BaseContainerTools.FindEntitySource(projectileResource);

		float timeToTarget;
		float targetDistance = vector.Distance(barrelOrigin, m_Target.GetAimPoint());
		float heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);

		Physics targetRB = m_Target.m_Ent.GetPhysics();

		if (targetRB)
		{
			//Add Leading
			predictedLeadingOffset = targetRB.GetVelocity() * timeToTarget;
			targetDistance = vector.Distance(barrelOrigin, m_Target.GetAimPoint() + predictedLeadingOffset);
			heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);
		}

		//Add Ballistics
		predictedLeadingOffset[1] = predictedLeadingOffset[1] + heightOffset;

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
		vector current = SCR_Math3D.FixEulerVector180(m_vCurrentAngles);
		vector target = SCR_Math3D.FixEulerVector180(m_vTargetAngles);

		return Math.AbsFloat(current[0] - target[0]) < m_fAngleTolerance
			&& Math.AbsFloat(current[1] - target[1]) < m_fAngleTolerance;
	}

	//------------------------------------------------------------------------------------------------
	//! We idle, just chilling
	void HandleIdle()
	{
		if (m_Target)
			m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
	}

	//------------------------------------------------------------------------------------------------
	void HandleRotatingToTarget(float timeSlice)
	{
		vector desiredAngles;

		if (m_Target)
		{
			vector barrelMat[4];
			GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat); //Local mat
			vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]); //World pos

			GetOwner().GetTransform(barrelMat); //Override mat
			barrelMat[3] = barrelOrigin; //Add origin back

			vector aimPoint = m_Target.GetAimPoint();

			#ifdef WORKBENCH
			if (m_bDebug)
				Shape.CreateSphere(Color.YELLOW, ShapeFlags.ONCE, aimPoint, 0.3);
			#endif

			if (m_TurretComp.m_eFireMode == BON_TurretFireMode.Intercept)
			{
				//Only apply ballistics if not a missile
				if (m_TurretComp.m_bIsMissile)
					aimPoint += ComputeLeadSimple();
				else
					aimPoint += ComputeLead();
			}

			#ifdef WORKBENCH
			if (m_bDebug)
				Shape.CreateSphere(Color.RED, ShapeFlags.ONCE, aimPoint, 0.3);
			#endif

			desiredAngles = SCR_Math3D.GetLocalAngles(barrelMat, aimPoint);
		}
		else
		{
			//No Target -> Idle pos
			desiredAngles = vector.Zero
		}

		RotateTo(desiredAngles, timeSlice);

		//Done rotating to idle pos
		if (!m_Target && IsOnTarget())
			m_eAimState = BON_TurretAimState.IDLE;
	}

	//------------------------------------------------------------------------------------------------
	//! Rotate to desired angles within limits
	void RotateTo(vector targetAngles, float timeSlice)
	{
		if (!IsWithinLimitsAngle(targetAngles))
			return;

		m_vTargetAngles = targetAngles;
		m_vCurrentAngles = SCR_Math3D.LerpAngle(m_vCurrentAngles, m_vTargetAngles, m_fRotationSpeed * timeSlice);

		m_SignalsManager.SetSignalValue(m_iSignalBody, -m_vCurrentAngles[0]);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_vCurrentAngles[1]);
	}

	//------------------------------------------------------------------------------------------------
	void GetBodyTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		vector localBoneMat[4];
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBodyBone), localBoneMat); //Local mat

		//Transform relative to turret
		vector ownerMat[4];
		GetOwner().GetWorldTransform(ownerMat);
		Math3D.MatrixMultiply4(ownerMat, localBoneMat, mat);
	}

	//------------------------------------------------------------------------------------------------
	void GetBarrelTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		vector localBoneMat[4];
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBarrelBone), localBoneMat); //Local mat

		//Transform relative to turret
		vector ownerMat[4];
		GetOwner().GetWorldTransform(ownerMat);
		Math3D.MatrixMultiply4(ownerMat, localBoneMat, mat);
	}

	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	void ShowDebug()
	{
		DebugTextWorldSpace.Create(GetOwner().GetWorld(), typename.EnumToString(BON_TurretAimState, m_eAimState),
			DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			GetOwner().GetOrigin()[0], GetOwner().GetOrigin()[1] + 5, GetOwner().GetOrigin()[2]
		);

		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(GetOwner().FindComponent(FactionAffiliationComponent));

		DebugTextWorldSpace.Create(
			GetOwner().GetWorld(),
			fac.GetAffiliatedFaction().GetFactionKey(),
			DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			GetOwner().GetOrigin()[0], GetOwner().GetOrigin()[1] + 5.5, GetOwner().GetOrigin()[2],
			20,
			fac.GetAffiliatedFaction().GetFactionColor().PackToInt()
		);

		vector dir = m_vTargetAngles.AnglesToVector().Normalized();
		vector barrelMat[4];
		GetBarrelTransform(barrelMat);

		Shape.CreateArrow(barrelMat[3], barrelMat[3] + dir * 5, 0.25, Color.BLUE, ShapeFlags.ONCE);

		if (m_Target && m_Target.m_Ent)
		{
			Shape.CreateSphere(Color.GREEN, ShapeFlags.ONCE, m_Target.m_Ent.GetOrigin(), 0.1);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE, m_Target.GetAimPoint(), 0.11);
		}
	}
	#endif

	//------------------------------------------------------------------------------------------------
	//! Server + Client
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		// Prevent firing on stale idle angles
		if (target != m_Target)
			m_vTargetAngles = Vector(-1, -1, -1);

		m_Target = target;
		switch (m_eAimState)
		{
			case BON_TurretAimState.IDLE:
				HandleIdle();
				break;

			case BON_TurretAimState.ROTATING_TO_TARGET:
				HandleRotatingToTarget(timeSlice);
				break;
		}

		#ifdef WORKBENCH
		if (m_bDebug)
			ShowDebug();
		#endif
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_TurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));
		m_iSignalBody = m_SignalsManager.AddOrFindSignal("BodyRotation", 0);
		m_iSignalBarrel = m_SignalsManager.AddOrFindSignal("BarrelRotation", 0);

		Animation anim = GetOwner().GetAnimation();
		if (anim)
			m_iBarrelBoneIndex = anim.GetBoneIndex(m_sBarrelBone);
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

		CreateCircleSlice(boneMat[3], -owner.GetTransformAxis(1).Normalized(), owner.GetTransformAxis(2).Normalized(),
			m_vLimitHorizontal[0], m_vLimitHorizontal[1], 5, Color.BLUE, 32, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP |ShapeFlags.ONCE);

	}
}
