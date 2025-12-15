enum BON_TurretFireMode
{
	Direct,
	Guided,
	Intercept
}

[ComponentEditorProps(category: "BON/Turrets", description: "Auto Aiming Turrets without AI Characters")]
class BON_AutoTurretComponentClass : ScriptComponentClass
{
	//------------------------------------------------------------------------------------------------
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};

		//requires.Insert(BON_AutoTurretTargetingComponent);
		//requires.Insert(BON_AutoTurretAimingComponent);

		return requires;
	}

	//------------------------------------------------------------------------------------------------
	static BON_AutoTurretComponent IsAutoTurret(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		BON_AutoTurretComponent autoTurretComp = BON_AutoTurretComponent.Cast(owner.FindComponent(BON_AutoTurretComponent));
		return autoTurretComp;
	}
}

class BON_AutoTurretComponent : ScriptComponent
{
	//--- SETUP ---
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Projectile to shoot (Currently only works with Missles)", "et", category: "Setup")]
	ResourceName m_Projectile;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Muzzle particle effect", "ptc", category: "Setup")]
	protected ResourceName m_sMuzzleParticle;

	[Attribute(desc: "Turret muzzles (Projectile spawn pos) E.g one for each barrel", category: "Setup")]
	protected ref array<ref PointInfo> m_ProjectileMuzzles;

	[Attribute("SOUND_SHOT", UIWidgets.Auto, "", category: "Setup")]
	string m_sShootSound;

	[Attribute("5", UIWidgets.Auto, "Every shot has this % chance to explode the projectile (missile) its shooting at", "0 100 1", category: "Setup")]
	int m_fProjectileTriggerChance;

	[Attribute("0", UIWidgets.CheckBox, "Trigger projectile near target?", category: "Setup")]
	bool m_bTriggerOnTarget;

	[Attribute("1", UIWidgets.CheckBox, "Enable turret on spawn?", category: "Setup")]
	bool m_bActiveOnSpawn;

	//--- SHOOTING ---
	[Attribute("1", UIWidgets.Auto, "Time / Cooldown between attacks (s). Lower = faster", "0 inf 0.1", category: "Muzzle")]
	float m_fAttackDelay;

	[Attribute("0.25", UIWidgets.Auto, "Random angles for projectiles. 0 = no inaccuracy", "0 inf 1", category: "Muzzle")]
	float m_fAttackInaccuracy;

	[Attribute("0", uiwidget: UIWidgets.ComboBox, "Direct: Aim straight at the target's current position | \ Guided: Follow the target (missiles only!) | Intercept: Lead the target to intercept at predicted position", enums: ParamEnumArray.FromEnum(BON_TurretFireMode), category: "Muzzle")]
	BON_TurretFireMode m_eFireMode;

	//--- MISC ---
	[Attribute("0", UIWidgets.CheckBox, "Enable debug?", category: "Debug")]
	bool m_bDebug;

	//[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iNearestTargetId;


	protected float m_fAttackTimer;
	protected int m_iCurrentMuzzle;
	bool m_bActive = false;

	protected ref Faction m_Faction;
	protected AnimationControllerComponent m_AnimationController;
	protected SignalsManagerComponent m_SignalsManager;

	protected int m_iSignalBody;
	protected int m_iSignalBarrel;
	protected int m_iShootCmd;
	protected int m_iBodyVar;
	protected float m_fProjectileSpeed;
	protected float m_iAttacksOnTarget;
	protected bool m_bIsProjectileReplicated;
	float m_fRocketGuidanceStrength = 0;

	protected int m_iTargetCount;
	protected int m_iCurrentTargetIndex;

	protected float m_fLerp;
	protected float m_fNewBodyYaw;
	protected float m_fNewBarrelPitch;
	protected float m_fCurrentBodyYaw;
	protected float m_fCurrentBarrelPitch;

	bool m_bOnTarget;
	protected ref BON_AutoTurretTarget m_Target;
	ref Shape m_LoSDebug;
	
	SoundComponent m_SoundComponent;	
	BON_AutoTurretAimingComponent m_AimingComp;
	BON_AutoTurretTargetingComponent m_TargetingComp;

	//------------------------------------------------------------------------------------------------
	void ToggleActive()
	{
		m_bActive = !m_bActive;

		if (m_bActive)
			ConnectToAutoTurretSystem();
		else
			DisconnectFromAutoTurretSystem();
	}

	//------------------------------------------------------------------------------------------------
	void SetTargetFlags(int newFlags)
	{
		m_TargetingComp.m_eTargetFlags = newFlags;
	}
	
	//------------------------------------------------------------------------------------------------
	BON_TurretTargetFilterFlags GetTargetFlags()
	{
		return m_TargetingComp.m_eTargetFlags;
	}
		
	//------------------------------------------------------------------------------------------------
	void TriggerProjectile(IEntity projectile)
	{
		if (!projectile)
			return;

		BaseTriggerComponent trigger = BaseTriggerComponent.Cast(projectile.FindComponent(BaseTriggerComponent));
		if (!trigger)
		{
			Print("[AutoTurretCore] No TriggerComponent on " + projectile, LogLevel.WARNING);
			m_bTriggerOnTarget = false;
			return;
		}

		trigger.SetLive();
		trigger.OnUserTriggerOverrideInstigator(projectile, Instigator.CreateInstigator(GetOwner()));
	}

	//------------------------------------------------------------------------------------------------
	void SetTriggerOnTarget(IEntity projectile)
	{
		float timeToTarget;
		float targetDistance = vector.Distance(GetOwner().GetOrigin(), m_Target.GetAimPoint());
		BallisticTable.GetHeightFromProjectile(targetDistance, timeToTarget, projectile);

		if (timeToTarget <= 0)
			return;
		
		timeToTarget += s_AIRandomGenerator.RandFloatXY(-0.2, 0.2);
		//SetTimer on TimerTriggerComponent does not work :(
		GetGame().GetCallqueue().CallLater(TriggerProjectile, timeToTarget * 1000, false, projectile); 
	}
	
	//------------------------------------------------------------------------------------------------
	void LaunchProjectile(IEntity projectile)
	{
		if (!projectile)
			return;
		
		BON_GuidedProjectile guidedProjectile = BON_GuidedProjectile.Cast(projectile);
		if (guidedProjectile)
		{
			if (m_fRocketGuidanceStrength != 0)
				guidedProjectile.m_fGuidanceStrength = m_fRocketGuidanceStrength;
			guidedProjectile.SetTargetAndLaunch(m_Target.m_Ent, m_eFireMode);
			return;
		}

		ProjectileMoveComponent moveComp = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (moveComp)
			moveComp.Launch(projectile.GetTransformAxis(2).Normalized(), vector.Zero, 1, projectile, GetOwner(), null, null, null);
	}

	//------------------------------------------------------------------------------------------------
	void PlayShootSound()
	{
		if (m_SoundComponent)
			m_SoundComponent.SoundEvent(m_sShootSound);
	}
	
	//------------------------------------------------------------------------------------------------
	void SpawnMuzzleParticle(vector muzzleMat[4])
	{
		if (!m_sMuzzleParticle)
			return;
		
		ParticleEffectEntitySpawnParams params();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = muzzleMat;
		ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_sMuzzleParticle, params);
	}
	
	//------------------------------------------------------------------------------------------------
	void GetNextMuzzleTransform(out vector muzzleMat[4])
	{
		PointInfo currentMuzzle = m_ProjectileMuzzles[m_iCurrentMuzzle];
		currentMuzzle.GetTransform(muzzleMat);
		
		m_iCurrentMuzzle++;
		if (m_iCurrentMuzzle >= m_ProjectileMuzzles.Count())
			m_iCurrentMuzzle = 0;
	}
	
	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		vector muzzleMat[4];
		GetNextMuzzleTransform(muzzleMat);
		SCR_Math3D.AddRandomVectorToMat(muzzleMat, -m_fAttackInaccuracy, m_fAttackInaccuracy);
		
		IEntity projectile = SpawnProjectile(muzzleMat, m_Target);
		SpawnMuzzleParticle(muzzleMat);
		PlayShootSound();		
		
		if (m_AnimationController)
			m_AnimationController.CallCommand(m_iShootCmd, 1, 0);
		
		if (m_bTriggerOnTarget)
			SetTriggerOnTarget(projectile);
		
		//TOOD: Maybe first few bullets never hit? To prevent first one to hit. Or increase chance every bullet up to max
		bool triggerTargetProjectile = s_AIRandomGenerator.RandIntInclusive(1, 100) < m_fProjectileTriggerChance;
		if (triggerTargetProjectile && m_Target.m_Ent.FindComponent(BaseTriggerComponent))
			TriggerProjectile(m_Target.m_Ent);
	}
	
	//------------------------------------------------------------------------------------------------
	IEntity SpawnProjectile(vector muzzleMat[4], BON_AutoTurretTarget target)
	{		
		EntitySpawnParams spawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = muzzleMat;

		if (!m_bIsProjectileReplicated || Replication.IsServer())
		{
			IEntity lastSpawnedProjectile = GetGame().SpawnEntityPrefab(Resource.Load(m_Projectile), GetGame().GetWorld(), spawnParams);
			if (lastSpawnedProjectile)
				LaunchProjectile(lastSpawnedProjectile);
			
			return lastSpawnedProjectile;
		}
		
		return null;
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdate(float timeSlice)
	{
		m_Target = m_TargetingComp.GetTarget();
		m_AimingComp.OnUpdate(m_Target, timeSlice);
		
		m_fAttackTimer -= timeSlice;
		if (m_fAttackTimer <= 0 && m_AimingComp.CanFire())
		{
			Fire();
			m_fAttackTimer = m_fAttackDelay;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ConnectToAutoTurretSystem()
	{
		World world = GetOwner().GetWorld();
		BON_AutoTurretSystem system = BON_AutoTurretSystem.Cast(world.FindSystem(BON_AutoTurretSystem));
		if (system)
			system.Register(this);
	}

	//------------------------------------------------------------------------------------------------
	protected void DisconnectFromAutoTurretSystem()
	{
		World world = GetOwner().GetWorld();
		BON_AutoTurretSystem system = BON_AutoTurretSystem.Cast(world.FindSystem(BON_AutoTurretSystem));
		if (system)
			system.Unregister(this);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		if (m_ProjectileMuzzles.IsEmpty() || !m_Projectile)
			return;

		foreach (PointInfo muzzle : m_ProjectileMuzzles)
		{
			muzzle.Init(owner);
		}

		if (!GetGame().InPlayMode())
			return;
		
		m_SoundComponent = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));		
		m_AimingComp = BON_AutoTurretAimingComponent.Cast(owner.FindComponent(BON_AutoTurretAimingComponent));
		m_TargetingComp = BON_AutoTurretTargetingComponent.Cast(owner.FindComponent(BON_AutoTurretTargetingComponent));		
		m_AnimationController = BaseItemAnimationComponent.Cast(owner.FindComponent(BaseItemAnimationComponent));
		m_iShootCmd = m_AnimationController.BindCommand("CMD_SHOOT");		

		Resource projectileResource = Resource.Load(m_Projectile);
		if (!projectileResource)
			return;

		BaseContainer rplCompBase = SCR_BaseContainerTools.FindComponentSource(projectileResource, RplComponent);
		m_bIsProjectileReplicated = (rplCompBase != null);
		BaseContainer projectileMoveComp = SCR_BaseContainerTools.FindComponentSource(projectileResource, ProjectileMoveComponent);
		projectileMoveComp.Get("InitSpeed", m_fProjectileSpeed);

		ConnectToAutoTurretSystem();
		m_bActive = m_bActiveOnSpawn;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnDeactivate(IEntity owner)
	{
		DisconnectFromAutoTurretSystem();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		DisconnectFromAutoTurretSystem();
	}

	//------------------------------------------------------------------------------------------------
	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		foreach (PointInfo muzzle : m_ProjectileMuzzles)
		{
			vector mat[4];
			muzzle.GetTransform(mat);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE | ShapeFlags.WIREFRAME, mat[3], 0.075);
		}
	}
}
