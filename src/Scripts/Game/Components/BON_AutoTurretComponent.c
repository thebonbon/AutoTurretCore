//Made by TheBonBon :)

enum BON_TurretTargetFilterFlags
{
	CHARACTERS = 1 << 0,
	VEHICLES = 1 << 1,
	AIRCRAFTS = 1 << 2,
	PROJECTILES = 1 << 3,
}

[ComponentEditorProps(category: "BON/Turrets", description: "Auto Aiming Turrets without AI Characters")]
class BON_AutoTurretComponentClass : ScriptComponentClass
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};

		requires.Insert(FactionAffiliationComponent);
		requires.Insert(SignalsManagerComponent);
		requires.Insert(AnimationControllerComponent);

		return requires;
	}
}

class BON_AutoTurretComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Projectile to shoot (Currently only works with Missles)", "et", category: "Setup")]
	ResourceName m_Projectile;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Muzzle particle effect", "ptc", category: "Setup")]
	protected ResourceName m_sMuzzleParticle;

	[Attribute(desc: "Turret muzzle (Projectile spawn pos)", category: "Setup")]
	protected ref PointInfo m_ProjectileMuzzle;

	[Attribute("SOUND_SHOT", UIWidgets.Auto, "", category: "Setup")]
	string m_sShootSound;

	[Attribute("50", UIWidgets.Auto, "How many shots need to be fired at a Projectile to (fake) explode it?", category: "Setup")]
	float m_iAttacksPerProjectile;

	[Attribute("w_body", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBodyBone;

	[Attribute("w_barrel", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBarrelBone;

	[Attribute("0 360", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitHorizontal;

	[Attribute("0 90", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitVertical;

	[Attribute(uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	protected BON_TurretTargetFilterFlags m_TargetFlags;

	[Attribute("1", UIWidgets.Auto, "Rotation Speed. Higher = faster. 0 = no rotation", "0 inf 1", category: "Aiming")]
	float m_fRotationSpeed;

	[Attribute("1", UIWidgets.Auto, "Attack cooldown (s). Lower = faster", "0 inf 1", category: "Aiming")]
	float m_fMaxAttackSpeed;

	[Attribute("500", UIWidgets.Auto, "Attack range (m)", category: "Aiming")]
	float m_fAttackRange;

	[Attribute("0.25", UIWidgets.Auto, "Random angles for projectiles. 0 = no inaccuracy", "0 inf 1", category: "Aiming")]
	float m_fAttackInaccuracy;

	[Attribute("0", UIWidgets.CheckBox, "Enable debug?", category: "Debug")]
	bool m_bDebug;


	static ref array<EVehicleType> m_aTargetVehicleTypes = {
		EVehicleType.APC,
		EVehicleType.CAR,
		EVehicleType.TRUCK,
		EVehicleType.FUEL_TRUCK,
		EVehicleType.COMM_TRUCK,
		EVehicleType.SUPPLY_TRUCK
	};

	protected float m_fNearestDist = float.MAX;
	protected float m_fAttackSpeed = 0;
	protected bool m_bActive = false;
	protected float m_fMaxSearchDelay = 1;

	protected IEntity m_LastTarget;
	protected ref Faction m_Faction;
	protected SignalsManagerComponent m_SignalsManager;
	protected AnimationControllerComponent m_AnimationController;
	protected PerceivableComponent m_TargetPerceivableComp;

	ref Resource m_ProjectileResource;
	protected IEntitySource m_ProjectileSource;
	protected int m_iSignalBody;
	protected int m_iSignalBarrel;
	protected int m_iShootCmd;
	protected int m_iBodyVar;
	protected TNodeId m_iBarrelBoneIndex;
	protected float m_fProjectileSpeed;
	protected float m_fSearchDelay = 0;
	protected float m_iAttacksOnTarget;
	protected bool m_bIsProjectileReplicated;

	protected float m_fLerp;
	protected float m_fNewBodyYaw;
	protected float m_fNewBarrelPitch;
	protected float m_fCurrectBodyYaw;
	protected float m_fCurrentBarrelPitch;

	[RplProp()]
	bool m_bOnTraget;

	[RplProp(onRplName: "OnTargetChanged")]
	protected RplId m_iNearestTargetId;
	protected RplId m_iNewTargetId;

	protected IEntity m_NearestTarget;
	protected IEntity m_TempNearestTarget;
	ref array<IEntity> m_aValidTargets = {};
	protected float m_fNearestDis = float.MAX;
	ref Shape m_LoSDebug;

	//------------------------------------------------------------------------------------------------
	void OnTargetChanged()
	{
		if (m_iNearestTargetId == -1)
			m_NearestTarget = null;
		else
		{
			RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(m_iNearestTargetId));
			if (rplComponent)
				m_NearestTarget = rplComponent.GetEntity();
			else
			{
				Print("[ATC] Failed to get rplComponent for ID: " + m_iNearestTargetId, LogLevel.WARNING);
				m_NearestTarget = null;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void ToggleActive()
	{
		m_bActive = !m_bActive;
	}

	//------------------------------------------------------------------------------------------------
	void LaunchMissile(notnull IEntity rocket, vector direction)
	{
		ProjectileMoveComponent moveComp = ProjectileMoveComponent.Cast(rocket.FindComponent(ProjectileMoveComponent));

		if (!moveComp)
			return;

		m_AnimationController.CallCommand(m_iShootCmd, 1, 0);

		moveComp.Launch(direction, vector.Zero, 1, rocket, GetOwner(), null, m_NearestTarget, null);
	}

	//------------------------------------------------------------------------------------------------
	void Aim(float timeSlice)
	{
		if (m_NearestTarget != m_LastTarget)
		{
			m_LastTarget = m_NearestTarget;
			m_fCurrectBodyYaw = m_fNewBodyYaw;
			m_fCurrentBarrelPitch = m_fNewBarrelPitch;
			m_fLerp = 0;
		}

		m_fLerp += timeSlice * m_fRotationSpeed;
		m_fLerp = Math.Clamp(m_fLerp, 0, 1);

		//Back to rest position
		if (!m_NearestTarget)
		{
			m_fNewBodyYaw = Math.Lerp(m_fCurrectBodyYaw, 0, m_fLerp);
			m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);

			m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, 0, m_fLerp);
			m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fNewBarrelPitch);

			if (m_fLerp == 1)
			{
				m_fLerp = 0;
				m_fCurrectBodyYaw = m_fNewBodyYaw;
				m_fCurrentBarrelPitch = m_fNewBarrelPitch;
			}
			return;
		}

		//Targeting
		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().GetOrigin() + barrelMat[3];
		float targetDistance = vector.Distance(barrelOrigin, m_NearestTarget.GetOrigin());
		vector targetAimPoint = m_NearestTarget.GetOrigin();

		//Aim Point
		if (m_TargetPerceivableComp)
		{
			array<vector> aimPoints();
			m_TargetPerceivableComp.GetAimpoints(aimPoints);
			targetAimPoint = aimPoints[0];
		}

		//Leading
		if (m_NearestTarget.GetPhysics())
			targetAimPoint += (m_NearestTarget.GetPhysics().GetVelocity() * targetDistance / m_fProjectileSpeed);

		vector targetDir = targetAimPoint - GetOwner().GetOrigin();
		targetDir[1] = 0;
		Math3D.MatrixNormalize(targetDir);

		//Body
		vector rotationMatrixBody;
		Math3D.MatrixFromForwardVec(targetDir, rotationMatrixBody);
		vector newBodyAngles = Math3D.MatrixToAngles(rotationMatrixBody);
		float targetBodyYaw = -newBodyAngles[0];
		m_fNewBodyYaw = Math.Lerp(m_fCurrectBodyYaw, targetBodyYaw, m_fLerp);
		m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);

		// Barrel
		float flightTime;
		float heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, flightTime, m_ProjectileSource);
		targetDistance = Math.Sqrt(
			(targetAimPoint[0] - barrelOrigin[0]) * (targetAimPoint[0] - barrelOrigin[0]) +
			(targetAimPoint[2] - barrelOrigin[2]) * (targetAimPoint[2] - barrelOrigin[2])
		);

		float targetDeltaY = (targetAimPoint[1] - barrelOrigin[1]) + heightOffset;
		float targetBarrelPitch = Math.RAD2DEG * Math.Atan2(targetDeltaY, targetDistance);
		m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, targetBarrelPitch, m_fLerp);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fNewBarrelPitch);
	}

	//------------------------------------------------------------------------------------------------
	//! Checks a single target in valid target list
	void FindNearestTarget()
	{
		if (!m_aValidTargets.IsEmpty())
		{
			IEntity target = m_aValidTargets.Get(0);
			m_aValidTargets.Remove(0);
			if (!target)
				return;

			//TODO: Make custom class for target with distance already in cause we dont need to do this twice
			float dis = vector.DistanceSq(target.GetOrigin(), GetOwner().GetOrigin());
			if (dis < m_fNearestDis && LineOfSightCheck(target))
			{
				m_fNearestDist = dis;
				m_TempNearestTarget = target;
			}
		}
		else //All targets checked
		{
			GetValidTargets();
			m_fMaxSearchDelay = 1;
			if (!m_aValidTargets.IsEmpty())
				m_fMaxSearchDelay = 0.5 / m_aValidTargets.Count(); //Checking all targets will always take 0.5 second

			m_NearestTarget = m_TempNearestTarget;
			m_fNearestDis = float.MAX;
			m_TempNearestTarget = null;

			if (m_NearestTarget)
			{
				m_TargetPerceivableComp = PerceivableComponent.Cast(m_NearestTarget.FindComponent(PerceivableComponent));
				RplComponent rplComp = RplComponent.Cast(m_NearestTarget.FindComponent(RplComponent));
				m_iNewTargetId = rplComp.Id();
			}
			else
				m_iNewTargetId = -1;

			if (m_iNearestTargetId != m_iNewTargetId)
			{
				m_iNearestTargetId = m_iNewTargetId;
				Replication.BumpMe();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void GetValidTargets()
	{
		m_aValidTargets.Clear();

		//Vehicles and Characters
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		core.GetAllEntities(entities);

		foreach (SCR_EditableEntityComponent entityComp : entities)
		{
			if (!entityComp.HasEntityFlag(EEditableEntityFlag.HAS_FACTION))
				continue;

			if (!IsEnemyAndAlive(entityComp.GetOwner()))
				continue;

			if (entityComp.GetEntityType() == EEditableEntityType.CHARACTER && !SCR_Enum.HasFlag(m_TargetFlags, BON_TurretTargetFilterFlags.CHARACTERS))
				continue;

			if (entityComp.GetEntityType() == EEditableEntityType.VEHICLE)
			{
				EVehicleType vehType = Vehicle.Cast(entityComp.GetOwner()).m_eVehicleType;
				if (m_aTargetVehicleTypes.Contains(vehType) && !SCR_Enum.HasFlag(m_TargetFlags, BON_TurretTargetFilterFlags.VEHICLES))
					continue;
				//No Helicopter type yet
				if (vehType == EVehicleType.VEHICLE && !SCR_Enum.HasFlag(m_TargetFlags, BON_TurretTargetFilterFlags.AIRCRAFTS))
					continue;
			}

			vector entityPos;
			if (!entityComp.GetPos(entityPos))
				continue;

			float dis = vector.DistanceSq(entityPos, GetOwner().GetOrigin());
			if (dis > m_fAttackRange)
				continue;

			m_aValidTargets.Insert(entityComp.GetOwner());
		}

		//Projectiles
		if (!SCR_Enum.HasFlag(m_TargetFlags, BON_TurretTargetFilterFlags.PROJECTILES))
			return;

		foreach (IEntity projectile : BON_ProjectileTrackingComponentClass.s_aTrackedProjectiles)
		{
			float dist = vector.DistanceSq(GetOwner().GetOrigin(), projectile.GetOrigin());
			if (dist > m_fAttackRange)
				continue;

			m_aValidTargets.Insert(projectile);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool LineOfSightCheck(IEntity ent)
	{
		vector muzzleMat[4];
		m_ProjectileMuzzle.GetTransform(muzzleMat);

		TraceParam param = new TraceParam();
		param.Start = muzzleMat[3];
		param.End = ent.GetOrigin(); //TODO: Add aimpoint
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.VISIBILITY;
		param.Exclude = GetOwner();
		param.LayerMask = EPhysicsLayerPresets.Projectile;
		float traceDistance = GetOwner().GetWorld().TraceMove(param, null);

		if (m_bDebug)
		{
			vector position = (param.End - param.Start) * traceDistance + param.Start;
			m_LoSDebug = Shape.CreateArrow(muzzleMat[3], position, 0.1, COLOR_GREEN, ShapeFlags.NOZBUFFER);
		}

		return (traceDistance == 1 || param.TraceEnt == ent);
	}

	//------------------------------------------------------------------------------------------------
	bool IsEnemyAndAlive(IEntity ent)
	{
		DamageManagerComponent dmgManager;
		FactionAffiliationComponent factionComp;
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(ent);
		if (character)
		{
			factionComp = character.m_pFactionComponent;
			dmgManager = character.GetDamageManager();
		}
		else
		{
			Vehicle veh = Vehicle.Cast(ent);
			if (veh)
			{
				factionComp = veh.m_pFactionComponent;
				dmgManager = veh.GetDamageManager();
			}
			else
			{
				factionComp = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
				dmgManager = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
			}
		}

		bool isValidTarget = (
			dmgManager &&
			!dmgManager.IsDestroyed() &&
			factionComp &&
			factionComp.GetAffiliatedFaction() &&
			factionComp.GetAffiliatedFaction().IsFactionEnemy(m_Faction)
		);

		return isValidTarget;

		//TODO: Fix
		/*
		vector mat[4];
		vector localUp = GetOwner().GetTransformAxis(1).Normalized();
		SCR_Math3D.LookAt(GetOwner().GetOrigin(), ent.GetOrigin(), localUp, mat);
		vector targetAngles = Math3D.MatrixToAngles(mat);
		bool inRangeHoriz = Math.IsInRange(targetAngles[0], m_vLimitHorizontal[0], m_vLimitHorizontal[1]);
		bool inRangeVert = Math.IsInRange(targetAngles[1], m_vLimitVertical[0], m_vLimitVertical[1]);

		return (inRangeHoriz && inRangeVert);
		*/
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		vector mat[4];
		m_ProjectileMuzzle.GetTransform(mat);
		vector inaccuracyAngles = Vector(
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy),
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy),
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy)
		);
		vector inaccuracyMat[3];
		Math3D.AnglesToMatrix(inaccuracyAngles, inaccuracyMat);
		Math3D.MatrixMultiply3(mat, inaccuracyMat, mat);

		vector direction = Math3D.MatrixToAngles(mat);
		EntitySpawnParams spawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = mat;

		IEntity lastSpawnedProjectile;
		if (!m_bIsProjectileReplicated || Replication.IsServer())
		{
			lastSpawnedProjectile = GetGame().SpawnEntityPrefab(Resource.Load(m_Projectile), GetGame().GetWorld(), spawnParams);
			LaunchMissile(lastSpawnedProjectile, direction);
		}

		SoundComponent soundComponent = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));
		if (soundComponent)
			soundComponent.SoundEvent(m_sShootSound);

		ParticleEffectEntitySpawnParams params();
		params.TransformMode = ETransformMode.WORLD;
		m_ProjectileMuzzle.GetTransform(params.Transform);
		ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_sMuzzleParticle, params);

		if (m_NearestTarget.FindComponent(MissileMoveComponent))
		{
			if (m_iAttacksOnTarget >= m_iAttacksPerProjectile)
			{
				BaseTriggerComponent trigger = BaseTriggerComponent.Cast(m_NearestTarget.FindComponent(BaseTriggerComponent));
				trigger.SetLive();
				trigger.OnUserTrigger(m_NearestTarget);
				m_iAttacksOnTarget = 0;
			}
			m_iAttacksOnTarget++;
		}
	}

	//------------------------------------------------------------------------------------------------
	void ShowDebug()
	{
		if (m_NearestTarget)
				Shape.CreateSphere(Color.RED, ShapeFlags.ONCE, m_NearestTarget.GetOrigin(), 1);
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdate(float timeSlice)
	{
		if (!m_bActive)
			return;


		if (m_bDebug)
			ShowDebug();

		if (RplSession.Mode() != RplMode.Client)
		{
			Aim(timeSlice);

			m_fSearchDelay -= timeSlice;
			if (m_fSearchDelay <= 0)
			{
				FindNearestTarget();
				m_fSearchDelay = m_fMaxSearchDelay;
			}

			if (m_fLerp >= 1 && !m_bOnTraget)
			{
				m_bOnTraget = true;
				Replication.BumpMe();
			} else if (m_fLerp < 1 && m_bOnTraget)
			{
				m_bOnTraget = false;
				Replication.BumpMe();
			}
		}

		m_fAttackSpeed -= timeSlice;
		if (m_fAttackSpeed <= 0 && m_NearestTarget && m_bOnTraget)
		{
			Fire();
			m_fAttackSpeed = m_fMaxAttackSpeed;
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
		if (!GetGame().InPlayMode())
			return;

		if (m_ProjectileMuzzle)
			m_ProjectileMuzzle.Init(owner);

		//Cause we calculate distance squared
		m_fAttackRange *= m_fAttackRange;

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionAffiliation.GetAffiliatedFaction();

		m_AnimationController = AnimationControllerComponent.Cast(owner.FindComponent(AnimationControllerComponent));
		m_iShootCmd = m_AnimationController.BindCommand("CMD_SHOOT");

		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_iSignalBody = m_SignalsManager.AddOrFindMPSignal("BodyRotation", 0.1, 1, 0, SignalCompressionFunc.RotDEG);
		m_iSignalBarrel = m_SignalsManager.AddOrFindMPSignal("BarrelRotation", 0.1, 1, 0, SignalCompressionFunc.RotDEG);

		m_iBarrelBoneIndex = GetOwner().GetAnimation().GetBoneIndex(m_sBarrelBone);

		if (m_Projectile)
		{
			m_ProjectileResource = Resource.Load(m_Projectile);
			if (m_ProjectileResource)
			{
				m_ProjectileSource = SCR_BaseContainerTools.FindEntitySource(m_ProjectileResource);
				BaseContainer rplCompBase = SCR_BaseContainerTools.FindComponentSource(m_ProjectileResource, RplComponent);
				m_bIsProjectileReplicated = (rplCompBase != null);
				BaseContainer shellMoveComp = SCR_BaseContainerTools.FindComponentSource(m_ProjectileResource, ProjectileMoveComponent);
				shellMoveComp.Get("InitSpeed", m_fProjectileSpeed);
			}
		}

		ConnectToAutoTurretSystem();

		m_bActive = true;
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
}
