enum BON_TurretAimState
{
	IDLE,
	ROTATING_TO_TARGET,
	ON_TARGET,
	RETURNING_TO_IDLE
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

	[Attribute("1", UIWidgets.Auto, desc: "", category: "Setup")]
	float m_fRotationSpeed;

	[Attribute("false", UIWidgets.CheckBox, "Show Aiming debug?", category: "Debug")]
	protected bool m_bDebug;
	
	SignalsManagerComponent m_SignalsManager;
	int m_iSignalBody;
	int m_iSignalBarrel;


	BON_TurretAimState m_eAimState = BON_TurretAimState.IDLE;
	vector m_vCurrentAngles;
	vector m_vTargetAngles;

	//------------------------------------------------------------------------------------------------
	bool IsWithinLimitsAngle(vector angles)
	{
		float yaw = angles[0];
		float pitch = angles[1];

		bool inHorizontal = Math.IsInRange(yaw, m_vLimitHorizontal[0], m_vLimitHorizontal[1]);
		bool inVertical = Math.IsInRange(pitch, m_vLimitVertical[0], m_vLimitVertical[1]);

		return true;
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

	/*
	//------------------------------------------------------------------------------------------------
	vector ComputeLead()
	{
		vector muzzleFwd = barrelMat[2].Normalized();
		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		Resource projectileResource = Resource.Load(m_Projectile);
		IEntitySource projectileSource = SCR_BaseContainerTools.FindEntitySource(projectileResource);

		if (m_eFireMode == BON_TurretFireMode.Intercept)
		{
			float timeToTarget;
			float targetDistance = vector.Distance(barrelOrigin, targetAimPoint);
			float heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);

			if (m_Target.GetPhysics())
			{
				//Add Leading
				targetAimPoint += m_Target.GetPhysics().GetVelocity() * timeToTarget;
				targetDistance = vector.Distance(barrelOrigin, targetAimPoint);
				heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, timeToTarget, projectileSource);
			}

			//Add Ballistics
			targetAimPoint[1] = targetAimPoint[1] + heightOffset;
		}
		return vector.Zero;
	}
	*/
	//------------------------------------------------------------------------------------------------
	bool IsOnTarget()
	{
		return false;
		return vector.Distance(m_vCurrentAngles, m_vTargetAngles) <= 2;
	}

	//------------------------------------------------------------------------------------------------
	//! Server + Client
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		DebugTextWorldSpace.Create(GetOwner().GetWorld(), typename.EnumToString(BON_TurretAimState, m_eAimState),
			DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			GetOwner().GetOrigin()[0], GetOwner().GetOrigin()[1] + 5, GetOwner().GetOrigin()[2]
		);

		switch (m_eAimState)
		{
			case BON_TurretAimState.IDLE:
				HandleIdle(target);
				break;

			case BON_TurretAimState.ROTATING_TO_TARGET:
				HandleRotatingToTarget(target, timeSlice);
				break;

			case BON_TurretAimState.ON_TARGET:
				HandleOnTarget(target);
				break;

			case BON_TurretAimState.RETURNING_TO_IDLE:
				HandleReturningToIdle(target, timeSlice);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	void HandleIdle(BON_AutoTurretTarget target)
	{
		if (target)
			m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
		
		//Maybe idle rotate here..
	}
	
	//------------------------------------------------------------------------------------------------
	void HandleRotatingToTarget(BON_AutoTurretTarget target, float timeSlice)
	{
		if (!target)
		{
			m_eAimState = BON_TurretAimState.RETURNING_TO_IDLE;
			return;
		}

		if (IsOnTarget())
		{
			m_eAimState = BON_TurretAimState.ON_TARGET;
			return;
		}

		vector mat[4];
		GetOwner().GetTransform(mat);
		vector angles = SCR_Math3D.GetLocalAngles(mat, target.GetAimPoint());
		vector dir = angles.AnglesToVector().Normalized();
		
#ifdef WORKBENCH
		if (m_bDebug)
		{
			Shape.CreateArrow(mat[3], mat[3] + dir * 5, 1, Color.BLUE, ShapeFlags.ONCE);
			Shape.CreateSphere(Color.GREEN, ShapeFlags.ONCE, target.m_Ent.GetOrigin(), 0.1);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE, target.GetAimPoint(), 0.11);
		}
#endif
		
		RotateTo(angles, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	void HandleOnTarget(BON_AutoTurretTarget target)
	{
		if (!target)
		{
			m_eAimState = BON_TurretAimState.RETURNING_TO_IDLE;
			return;
		}
		
		if (!IsOnTarget())
		{
			m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
			return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void HandleReturningToIdle(BON_AutoTurretTarget target, float timeSlice)
	{
		if (target) //Got new target while returning to idle
		{
			m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
			return;
		}
		
		if (IsOnTarget()) //Is in idle pos
		{
			m_eAimState = BON_TurretAimState.IDLE;
			return;
		}
		
		RotateTo(vector.Zero, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	//! Rotate to desired angles
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
	override void EOnInit(IEntity owner)
	{
		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_iSignalBody = m_SignalsManager.AddOrFindSignal("BodyRotation", 0);
		m_iSignalBarrel = m_SignalsManager.AddOrFindSignal("BarrelRotation", 0);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
}
