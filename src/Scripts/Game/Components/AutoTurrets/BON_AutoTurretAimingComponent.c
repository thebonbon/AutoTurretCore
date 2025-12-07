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
		
		return (inHorizontal && inVertical);
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsWithinLimitsPos(vector targetPosition)
	{
		vector dir = targetPosition - GetOwner().GetOrigin();
		vector angles = dir.VectorToAngles().MapAngles();
		
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
		return vector.DistanceSq(m_vCurrentAngles, m_vTargetAngles) <= 2;
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Server + Client
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		DebugTextWorldSpace.Create(GetOwner().GetWorld(), typename.EnumToString(BON_TurretAimState, m_eAimState),
		 DebugTextFlags.ONCE | DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA, GetOwner().GetOrigin()[0], GetOwner().GetOrigin()[1] + 5, GetOwner().GetOrigin()[2]
		);
		 
		
		switch (m_eAimState)
		{
			case BON_TurretAimState.IDLE:			
				if (target)
					m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
				break;
			
			case BON_TurretAimState.ROTATING_TO_TARGET:
				vector dir = target.m_Ent.GetOrigin() - GetOwner().GetOrigin();
				vector angles = dir.VectorToAngles().MapAngles();

				RotateTo(angles, timeSlice);
						
				if (IsOnTarget())
					m_eAimState = BON_TurretAimState.ON_TARGET;
				
				if (!target)
					m_eAimState = BON_TurretAimState.RETURNING_TO_IDLE;
			
			case BON_TurretAimState.ON_TARGET:
				if (!target)
					m_eAimState = BON_TurretAimState.RETURNING_TO_IDLE;
				break;
			
			case BON_TurretAimState.RETURNING_TO_IDLE:
				RotateTo(vector.Zero, timeSlice);
			
				if (target)
					m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
				else if (IsOnTarget())
					m_eAimState = BON_TurretAimState.IDLE;
			
				break;
		}
		
	}
	
	//------------------------------------------------------------------------------------------------
	//! Rotate to desired angles
	void RotateTo(vector targetAngles, float timeSlice)
	{
		if (!IsWithinLimitsAngle(targetAngles))
			return;
	
		m_vTargetAngles = targetAngles;
		
		//Works with Lerp cause it uses delta..
		m_vCurrentAngles = SCR_Math3D.LerpAngle(m_vCurrentAngles, targetAngles, m_fRotationSpeed * timeSlice);
		
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
