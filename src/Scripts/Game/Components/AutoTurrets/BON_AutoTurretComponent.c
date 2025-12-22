enum BON_TurretFireMode
{
	Direct,
	Intercept
}

[ComponentEditorProps(category: "BON/Turrets", description: "Auto Aiming Turrets without AI Characters")]
class BON_AutoTurretComponentClass : ScriptComponentClass
{
	//------------------------------------------------------------------------------------------------
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};

		requires.Insert(BON_AutoTurretTargetingComponent);
		requires.Insert(BON_AutoTurretAimingComponent);

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
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Projectile to shoot", "et", category: "Setup")]
	ResourceName m_Projectile;

	[Attribute(desc: "PointInfo: Projectile spawn positions (loops automatically)", category: "Setup")]
	protected ref array<ref PointInfo> m_ProjectileSpawnPositions;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Particle effect to play", "ptc", category: "Setup")]
	ResourceName m_MuzzleParticle;

	[Attribute(desc: "Effect positions (count and order needs to be the same as ProjectileSpawnPositions)", category: "Setup")]
	protected ref array<ref PointInfo> m_EffectPositions;

	[Attribute("SOUND_SHOT", UIWidgets.Auto, "Sound Name set in SoundComponent", category: "Setup")]
	string m_sShootSound;

	[Attribute("5", UIWidgets.Auto, "Every shot has this chance (%) to explode the projectile its shooting at", "0 100 1", category: "Setup")]
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

	[Attribute("0", uiwidget: UIWidgets.ComboBox, "Direct: Aim straight at the target's current position | Intercept: Lead the target to intercept at predicted position", enumType: BON_TurretFireMode, category: "Muzzle")]
	BON_TurretFireMode m_eFireMode;

	//--- MISC ---
	[Attribute("0", UIWidgets.CheckBox, "Enable debug?", category: "Debug")]
	bool m_bDebug;

	//[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iNearestTargetId;

	protected AnimationControllerComponent m_AnimationController;
	protected SoundComponent m_SoundComponent;
	BON_AutoTurretAimingComponent m_AimingComp;
	BON_AutoTurretTargetingComponent m_TargetingComp;

	protected Faction m_Faction;
	protected ref BON_AutoTurretTarget m_Target;
	protected int m_iShootCmd;
	float m_fProjectileSpeed;
	protected bool m_bIsProjectileReplicated;
	protected float m_fAttackTimer;
	protected int m_iCurrentMuzzle;
	bool m_bIsMissile;

	//GM Settings
	float m_fRocketGuidanceStrength = 0;
	bool m_bActive = false;


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
	//! Intercept (Missile)
	void LaunchGuided(BON_GuidedProjectile guidedProjectile)
	{
		//Override guidance strength (set via GM)
		if (m_fRocketGuidanceStrength != 0)
				guidedProjectile.m_fGuidanceStrength = m_fRocketGuidanceStrength;

		guidedProjectile.SetTargetAndLaunch(m_Target.m_Ent, m_eFireMode, m_fProjectileSpeed);
	}

	//------------------------------------------------------------------------------------------------
	//! Direct (Missile / Bullet)
	void LaunchDirect(IEntity projectile)
	{
		ProjectileMoveComponent moveComp = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (moveComp)
			moveComp.Launch(projectile.GetTransformAxis(2), vector.Zero, 1, projectile, GetOwner(), null, null, null);
	}

	//------------------------------------------------------------------------------------------------
	//! Bullet + Missiles
	void LaunchProjectile(IEntity projectile)
	{
		if (!projectile)
			return;

		BON_GuidedProjectile guidedProjectile = BON_GuidedProjectile.Cast(projectile);

		if (!guidedProjectile || m_eFireMode == BON_TurretFireMode.Direct)
			LaunchDirect(projectile);
		else
			LaunchGuided(guidedProjectile);

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
		ParticleEffectEntitySpawnParams params();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = muzzleMat;
		ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_MuzzleParticle, params);
	}

	//------------------------------------------------------------------------------------------------
	void GetMuzzleTransform(out vector spawnMat[4], out vector effectMat[4])
	{
		PointInfo spawnPosition = m_ProjectileSpawnPositions[m_iCurrentMuzzle];
		PointInfo effectPosition = m_EffectPositions[m_iCurrentMuzzle];
		spawnPosition.GetTransform(spawnMat);
		effectPosition.GetTransform(effectMat);
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		vector muzzleMat[4];
		vector effectMat[4];
		GetMuzzleTransform(muzzleMat, effectMat);
		SCR_Math3D.AddRandomVectorToMat(muzzleMat, -m_fAttackInaccuracy, m_fAttackInaccuracy);

		IEntity projectile = SpawnProjectile(muzzleMat, m_Target);
		LaunchProjectile(projectile);
		SpawnMuzzleParticle(effectMat);
		PlayShootSound();

		if (m_AnimationController)
			m_AnimationController.CallCommand(m_iShootCmd, 1, 0);

		if (m_bTriggerOnTarget)
			SetTriggerOnTarget(projectile);

		//TOOD: Maybe first few bullets never hit? To prevent first one to hit. Or increase chance every bullet up to max
		bool triggerTargetProjectile = s_AIRandomGenerator.RandIntInclusive(1, 100) < m_fProjectileTriggerChance;
		if (triggerTargetProjectile && m_Target.m_Ent.FindComponent(BaseTriggerComponent))
			TriggerProjectile(m_Target.m_Ent);

		//Increase muzzle index
		m_iCurrentMuzzle++;
		if (m_iCurrentMuzzle >= m_ProjectileSpawnPositions.Count())
			m_iCurrentMuzzle = 0;
	}

	//------------------------------------------------------------------------------------------------
	IEntity SpawnProjectile(vector muzzleMat[4], BON_AutoTurretTarget target)
	{
		EntitySpawnParams spawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = muzzleMat;

		IEntity lastSpawnedProjectile;

		if (!m_bIsProjectileReplicated || Replication.IsServer())
		{
			lastSpawnedProjectile = GetGame().SpawnEntityPrefab(Resource.Load(m_Projectile), GetGame().GetWorld(), spawnParams);
			if (!lastSpawnedProjectile)
				return null;

			//Update projectile faction of IFF
			BON_AutoTurretTargetComponent targetComp = BON_AutoTurretTargetComponent.Cast(lastSpawnedProjectile.FindComponent(BON_AutoTurretTargetComponent));
			if (targetComp)
				targetComp.m_Faction = m_Faction;
		}

		return lastSpawnedProjectile;
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdate(float timeSlice)
	{
		if (!m_bActive)
			return;

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
		if (m_ProjectileSpawnPositions.IsEmpty() || !m_Projectile)
			return;

		foreach (PointInfo spawnPos : m_ProjectileSpawnPositions)
		{
			spawnPos.Init(owner);
		}

		foreach (PointInfo effectPos : m_EffectPositions)
		{
			effectPos.Init(owner);
		}

		if (!GetGame().InPlayMode())
			return;

		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionComp.GetAffiliatedFaction();

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

		BaseContainer missileMoveComp = SCR_BaseContainerTools.FindComponentSource(projectileResource, MissileMoveComponent);
		if (missileMoveComp)
			m_bIsMissile = true;

		m_bActive = m_bActiveOnSpawn;
		if (m_bActive)
			ConnectToAutoTurretSystem();
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
		if (!m_bDebug)
			return;

		foreach (PointInfo spawnPos : m_ProjectileSpawnPositions)
		{
			vector mat[4];
			spawnPos.GetTransform(mat);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE | ShapeFlags.WIREFRAME, mat[3], 0.075);
		}
	}
}
