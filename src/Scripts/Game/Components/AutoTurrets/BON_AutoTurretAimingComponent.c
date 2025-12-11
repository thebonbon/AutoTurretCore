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

	[Attribute("w_body", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBodyBone;
		
	[Attribute("w_barrel", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBarrelBone;
	
	
	[Attribute("false", UIWidgets.CheckBox, "Show Aiming debug?", category: "Debug")]
	protected bool m_bDebug;

	SignalsManagerComponent m_SignalsManager;
	int m_iSignalBody;
	int m_iSignalBarrel;

	protected TNodeId m_iBarrelBoneIndex;

	
	BON_TurretAimState m_eAimState = BON_TurretAimState.IDLE;
	vector m_vCurrentAngles;
	vector m_vTargetAngles;

	//TODO
	/*
	//------------------------------------------------------------------------------------------------
	void SetNewTarget(IEntity target)
	{
		m_Target = target;
		if (target)
		{
			RplComponent targetRplComp = RplComponent.Cast(target.FindComponent(RplComponent));
			m_iNearestTargetId = targetRplComp.Id();
			m_TargetPerceivableComp = PerceivableComponent.Cast(target.FindComponent(PerceivableComponent));
		}
		else
		{
			m_iNearestTargetId = -1;
			m_TargetPerceivableComp = null;
		}

		Replication.BumpMe(); //Triggers OnTargetChanged()

		//Apply current rotation
		m_fCurrentBodyYaw = m_fNewBodyYaw;
		m_fCurrentBarrelPitch = m_fNewBarrelPitch;
		m_fLerp = 0;
		
	}
	
	
	//------------------------------------------------------------------------------------------------
	void OnTargetChanged()
	{
		if (m_iNearestTargetId == -1)
		{
			m_Target = null;
			m_TargetPerceivableComp = null;
		}
		else
		{
			RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(m_iNearestTargetId));
			if (rplComponent)
			{
				m_Target = rplComponent.GetEntity();
				m_TargetPerceivableComp = PerceivableComponent.Cast(rplComponent.GetEntity().FindComponent(PerceivableComponent));
			}
			else
			{

				Print("[AutoTurretCore] Failed to get rplComponent for ID: " + m_iNearestTargetId, LogLevel.WARNING);
				m_Target = null;
			}
	}
	*/
	
	
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
	bool ReadyToFire()
	{
		return (m_eAimState == BON_TurretAimState.ROTATING_TO_TARGET && IsOnTarget());
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsOnTarget()
	{
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

		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat); //Local mat
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]); //World mat
		
		GetOwner().GetTransform(barrelMat); //Override mat
		barrelMat[3] = barrelOrigin; //Add origin back
		
		vector angles = SCR_Math3D.GetLocalAngles(barrelMat, target.GetAimPoint());
		vector dir = angles.AnglesToVector().Normalized();

#ifdef WORKBENCH
		if (m_bDebug)
		{
			Shape.CreateArrow(barrelMat[3], barrelMat[3] + dir * 5, 1, Color.BLUE, ShapeFlags.ONCE);
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
		
		m_iBarrelBoneIndex = GetOwner().GetAnimation().GetBoneIndex(m_sBarrelBone);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	void GetBodyTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBodyBone), mat);
	}
	
	//------------------------------------------------------------------------------------------------
	void GetBarrelTransform(out vector mat[4])
	{
		Animation ownerAnim = GetOwner().GetAnimation();
		ownerAnim.GetBoneMatrix(ownerAnim.GetBoneIndex(m_sBarrelBone), mat);
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
