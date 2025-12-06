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
	
	float m_fRotationSpeed;
	
	SignalsManagerComponent m_SignalsManager;
	int m_iSignalBody;
	int m_iSignalBarrel;
	
	
	BON_TurretAimState m_eAimState = BON_TurretAimState.IDLE;
	vector m_vCurrentAngles;
	vector m_vTargetAngles;
	
	//------------------------------------------------------------------------------------------------
	bool IsWithinLimits(vector targetPosition)
	{
		vector angles = SCR_Math3D.AnglesFromTo(GetOwner().GetOrigin(), targetPosition);

		bool inVertical = -angles[0] >= m_vLimitVertical[0] && angles[0] <= m_vLimitVertical[1];
		bool inHorizontal = angles[1] >= m_vLimitHorizontal[0] && angles[1] <= m_vLimitHorizontal[1];
		
		return (inHorizontal && inVertical);			
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
		return vector.DistanceSq(m_vCurrentAngles, m_vTargetAngles) <= 2;
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Server + Client
	//! Called from main AutoTurretComponent
	void OnUpdate(BON_AutoTurretTarget target, float timeSlice)
	{
		switch (m_eAimState)
		{
			case BON_TurretAimState.IDLE:			
				if (target)
					m_eAimState = BON_TurretAimState.ROTATING_TO_TARGET;
				break;
			
			case BON_TurretAimState.ROTATING_TO_TARGET:
				vector angles = SCR_Math3D.AnglesFromTo(GetOwner().GetOrigin(), target.GetAimPoint());
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
		//if (!IsWithinLimits(targetPosition))
		//			return;
	
		m_vTargetAngles = targetAngles;	
		m_vCurrentAngles = SCR_Math3D.MoveTowards(m_vCurrentAngles, targetAngles, timeSlice * m_fRotationSpeed);		
		
		m_SignalsManager.SetSignalValue(m_iSignalBody, m_vCurrentAngles[0]);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_vCurrentAngles[1]);		
	}	
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
}
