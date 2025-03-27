//Made by TheBonBon :)

[ComponentEditorProps(category: "BON/Turrets", description: "Auto Aiming Turrets without AI Characters")]
class BON_AutoTurretComponentClass : ScriptComponentClass
{
	//------------------------------------------------------------------------------------------------
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = {};

		requires.Insert(FactionAffiliationComponent);
		requires.Insert(SignalsManagerComponent);
		requires.Insert(AnimationControllerComponent);

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

	[Attribute("w_body", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBodyBone;

	[Attribute("w_barrel", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBarrelBone;

	[Attribute("0 360", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitHorizontal;

	[Attribute("0 90", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitVertical;

	[Attribute(uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	BON_TurretTargetFilterFlags m_TargetFlags;

	[Attribute("1", UIWidgets.CheckBox, "Enable turret on spawn?", category: "Aiming")]
	bool m_bActiveOnSpawn;

	[Attribute("1", UIWidgets.Auto, "Rotation Speed. Higher = faster. 0 = no rotation", "0 inf 1", category: "Aiming")]
	float m_fRotationSpeed;

	[Attribute("1", UIWidgets.Auto, "Attack cooldown (s). Lower = faster", "0 inf 1", category: "Aiming")]
	float m_fMaxAttackSpeed;

	[Attribute("500", UIWidgets.Auto, "Attack range (m)", category: "Aiming")]
	int m_iAttackRange;

	[Attribute("0.25", UIWidgets.Auto, "Random angles for projectiles. 0 = no inaccuracy", "0 inf 1", category: "Aiming")]
	float m_fAttackInaccuracy;

	[Attribute("1", UIWidgets.CheckBox, "Enable leading? (Shooting in front of the target to account for velocity)", category: "Aiming")]
	bool m_bLeading;

	[Attribute("1", UIWidgets.CheckBox, "Enable ballistics offset? (Shooting above the target to account for projectile ballistics)", category: "Aiming")]
	bool m_bBallistics;

	[Attribute("0", UIWidgets.CheckBox, "Enable debug?", category: "Debug")]
	bool m_bDebug;

	[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iNearestTargetId;

	static ref array<EVehicleType> m_aTargetVehicleTypes = {
		EVehicleType.APC,
		EVehicleType.CAR,
		EVehicleType.TRUCK,
		EVehicleType.FUEL_TRUCK,
		EVehicleType.COMM_TRUCK,
		EVehicleType.SUPPLY_TRUCK
	};

	protected int m_iCurrentMuzzle;
	protected float m_fAttackSpeed = 0;
	bool m_bActive = false;

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
	protected float m_fSearchDelay;
	protected float m_fMaxSearchDelay = 1;
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

	protected IEntity m_NearestTarget;
	ref array<ref BON_AutoTurretTarget> m_aValidTargets = {};
	protected float m_fNearestDis = float.MAX;
	ref Shape m_LoSDebug;

	//------------------------------------------------------------------------------------------------
	void OnTargetChanged()
	{
		if (m_iNearestTargetId == -1)
		{
			m_NearestTarget = null;
			m_TargetPerceivableComp = null;
		}
		else
		{
			RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(m_iNearestTargetId));
			if (rplComponent)
			{
				m_NearestTarget = rplComponent.GetEntity();
				m_TargetPerceivableComp = PerceivableComponent.Cast(rplComponent.GetEntity().FindComponent(PerceivableComponent));
			}
			else
			{
				Print("[AutoTurretCore] Failed to get rplComponent for ID: " + m_iNearestTargetId, LogLevel.WARNING);
				m_NearestTarget = null;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	int GetAttackRange()
	{
		return Math.Sqrt(m_iAttackRange);
	}

	//------------------------------------------------------------------------------------------------
	void SetAttackRange(int range)
	{
		m_iAttackRange = Math.Pow(range, 2);
	}

	//------------------------------------------------------------------------------------------------
	bool ToggleActive()
	{
		m_bActive = !m_bActive;

		if (m_bActive)
			ConnectToAutoTurretSystem();
		else
			DisconnectFromAutoTurretSystem();

		return m_bActive;
	}

	//------------------------------------------------------------------------------------------------
	void LaunchProjectile(notnull IEntity rocket)
	{
		BON_GuidedProjectile guidedProjectile = BON_GuidedProjectile.Cast(rocket);
		if (guidedProjectile)
		{
			if (m_fRocketGuidanceStrength != 0)
				guidedProjectile.m_fGuidanceStrength = m_fRocketGuidanceStrength;
			guidedProjectile.SetTargetAndLaunch(m_NearestTarget);
			return;
		}

		ProjectileMoveComponent moveComp = ProjectileMoveComponent.Cast(rocket.FindComponent(ProjectileMoveComponent));

		if (!moveComp)
			return;

		m_AnimationController.CallCommand(m_iShootCmd, 1, 0);

		moveComp.Launch(rocket.GetTransformAxis(2), vector.Zero, 1, rocket, GetOwner(), null, null, null);
	}

	//------------------------------------------------------------------------------------------------
	void SetOnTarget(bool newValue)
	{
		m_bOnTarget = newValue;
	}

	//------------------------------------------------------------------------------------------------
	//! Server + Client only
	void Aim(float timeSlice)
	{
		m_fLerp += timeSlice * m_fRotationSpeed;
		m_fLerp = Math.Clamp(m_fLerp, 0, 1);

		//Back to rest position
		if (!m_NearestTarget)
		{
			SetOnTarget(false);

			m_fNewBodyYaw = Math.Lerp(m_fCurrentBodyYaw, 0, m_fLerp);
			m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);

			m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, 0, m_fLerp);
			m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fNewBarrelPitch);

			if (m_fLerp == 1)
			{
				m_fLerp = 0;
				m_fCurrentBodyYaw = m_fNewBodyYaw;
				m_fCurrentBarrelPitch = m_fNewBarrelPitch;
			}
			return;
		}

		//Inital source and target points
		vector targetAimPoint = m_NearestTarget.GetOrigin();

		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		//Add Aimpoint
		if (m_TargetPerceivableComp)
		{
			array<vector> aimPoints();
			m_TargetPerceivableComp.GetAimpoints(aimPoints);
			targetAimPoint = aimPoints[0];
		}

		//Add Leading
		float targetDistance = vector.Distance(barrelOrigin, targetAimPoint);
		if (m_bLeading && m_NearestTarget.GetPhysics())
		{
			float timeToTarget = targetDistance / m_fProjectileSpeed;
			targetAimPoint += m_NearestTarget.GetPhysics().GetVelocity() * timeToTarget;
			targetDistance = vector.Distance(barrelOrigin, targetAimPoint); //Re-calculate distance for leading point
		}

		//Add Ballistics
		if (m_bBallistics)
		{
			float heightOffset = BallisticTable.GetHeightFromProjectileSource(targetDistance, null, m_ProjectileSource);
			targetAimPoint = targetAimPoint + vector.Up * heightOffset;
		}

		//Set Signals
		vector targetDir = targetAimPoint - barrelOrigin;
		vector localTargetDir = GetOwner().VectorToLocal(targetDir);
		vector angles = localTargetDir.VectorToAngles();
		angles = SCR_Math3D.FixEulerVector180(angles);

		m_fNewBodyYaw = Math.Lerp(m_fCurrentBodyYaw, -angles[0], m_fLerp);
		m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, angles[1], m_fLerp);

		m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fNewBarrelPitch);

		if (m_fLerp == 1)
		{
			SetOnTarget(true);
			m_fLerp = 0;
			m_fCurrentBodyYaw = m_fNewBodyYaw;
			m_fCurrentBarrelPitch = m_fNewBarrelPitch;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Checks a single target in valid target list
	void CheckNextTarget()
	{
		if (m_iCurrentTargetIndex >= m_iTargetCount)
		{
			m_iCurrentTargetIndex = 0;
			m_iTargetCount = 0;
			GetValidTargets();
		}

		if (!m_aValidTargets.IsEmpty())
		{
			BON_AutoTurretTarget targetData = m_aValidTargets.Get(m_iCurrentTargetIndex);

			if (LineOfSightCheck(targetData.m_Ent))
				SetNewTarget(targetData.m_Ent);

			m_iCurrentTargetIndex++;
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetNewTarget(IEntity target)
	{
		//Reset old target vars
		if (m_NearestTarget)
		{
			SoundComponent soundComp = SoundComponent.Cast(m_NearestTarget.FindComponent(SoundComponent));
			if (soundComp)
				soundComp.SetSignalValue(soundComp.GetSignalIndex("TrackingState"), 0);
		}

		m_NearestTarget = target;

		if (target)
		{
			RplComponent targetRplComp = RplComponent.Cast(target.FindComponent(RplComponent));
			m_iNearestTargetId = targetRplComp.Id();
			m_TargetPerceivableComp = PerceivableComponent.Cast(target.FindComponent(PerceivableComponent));

			SoundComponent soundComp = SoundComponent.Cast(target.FindComponent(SoundComponent));
			if (soundComp)
			{
				soundComp.SetSignalValue(soundComp.GetSignalIndex("TrackingState"), 1);
				soundComp.SoundEvent("SOUND_TARGET_BEEP");
			}
		}
		else
		{
			m_iNearestTargetId = -1;
			m_TargetPerceivableComp = null;
		}

		Replication.BumpMe();

		//Apply current rotation
		m_fCurrentBodyYaw = m_fNewBodyYaw;
		m_fCurrentBarrelPitch = m_fNewBarrelPitch;
		m_fLerp = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Gets a broad list of possible targets
	//! Has to be alive + faction enemy + in target filter + in attack range
	void GetValidTargets()
	{
		m_aValidTargets.Clear();

		array<IEntity> allTargets = {};
		if (SCR_Enum.HasPartialFlag(m_TargetFlags, BON_TurretTargetFilterFlags.CHARACTERS))
			allTargets.InsertAll(BON_AutoTurretTargets.s_aTargetCharacters);
		if (SCR_Enum.HasPartialFlag(m_TargetFlags, BON_TurretTargetFilterFlags.VEHICLES))
			allTargets.InsertAll(BON_AutoTurretTargets.s_aTargetVehicles);
		if (SCR_Enum.HasPartialFlag(m_TargetFlags, BON_TurretTargetFilterFlags.AIRCRAFTS))
			allTargets.InsertAll(BON_AutoTurretTargets.s_aTargetAircrafts);
		if (SCR_Enum.HasPartialFlag(m_TargetFlags, BON_TurretTargetFilterFlags.PROJECTILES))
			allTargets.InsertAll(BON_AutoTurretTargets.s_aTargetProjectiles);

		foreach (IEntity target : allTargets)
		{
			if (!target)
				continue;

			float distance = vector.DistanceSq(target.GetOrigin(), GetOwner().GetOrigin());
			if (distance < m_iAttackRange && IsEnemyAndAlive(target))
				AddNewTarget(target, distance);
		}
	}

	//------------------------------------------------------------------------------------------------
	void AddNewTarget(IEntity ent, float distance)
	{
		BON_AutoTurretTarget newTarget = new BON_AutoTurretTarget(ent, distance);
		bool inserted;

		if (m_aValidTargets.IsEmpty())
			m_aValidTargets.Insert(newTarget);
		else
		{
			foreach (int i, BON_AutoTurretTarget target : m_aValidTargets)
			{
				if (distance < target.m_fDistance)
				{
					m_aValidTargets.InsertAt(target, i);
					inserted = true;
					break;
				}
			}
		}

		//Biggest dist, insert at the end
		if (!inserted)
			m_aValidTargets.Insert(newTarget);

		m_iTargetCount++;
	}

	//------------------------------------------------------------------------------------------------
	bool LineOfSightCheck(IEntity ent)
	{
		if (!ent)
			return false;

		vector muzzleMat[4];
		m_ProjectileMuzzles[m_iCurrentMuzzle].GetTransform(muzzleMat);

		vector targetAimPoint = ent.GetOrigin();

		//Add Aimpoint. Little up offset to not hit ground
		if (!Projectile.Cast(ent))
			targetAimPoint += vector.Up;

		if (m_TargetPerceivableComp)
		{
			array<vector> aimPoints();
			m_TargetPerceivableComp.GetAimpoints(aimPoints);
			targetAimPoint = aimPoints[0];
		}

		TraceParam param = new TraceParam();
		param.Start = muzzleMat[3];
		param.End = targetAimPoint;
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
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
		if (Projectile.Cast(ent))
			return true;

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
	}

	//------------------------------------------------------------------------------------------------
	void TriggerProjectile(IEntity projectile)
	{
		if (!projectile)
			return;

		BaseTriggerComponent trigger = BaseTriggerComponent.Cast(projectile.FindComponent(BaseTriggerComponent));
		trigger.SetLive();
		trigger.OnUserTrigger(projectile);
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		vector mat[4];
		m_ProjectileMuzzles[m_iCurrentMuzzle].GetTransform(mat);
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
			LaunchProjectile(lastSpawnedProjectile);
		}

		SoundComponent soundComponent = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));
		if (soundComponent)
			soundComponent.SoundEvent(m_sShootSound);

		if (m_sMuzzleParticle)
		{
			ParticleEffectEntitySpawnParams params();
			params.TransformMode = ETransformMode.WORLD;
			m_ProjectileMuzzles[m_iCurrentMuzzle].GetTransform(params.Transform);
			ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_sMuzzleParticle, params);
		}

		if (m_NearestTarget && m_NearestTarget.FindComponent(MissileMoveComponent) && s_AIRandomGenerator.RandIntInclusive(1, 100) < m_fProjectileTriggerChance)
			TriggerProjectile(m_NearestTarget);


		m_iCurrentMuzzle++;
		if (m_iCurrentMuzzle >= m_ProjectileMuzzles.Count())
			m_iCurrentMuzzle = 0;
	}

	//------------------------------------------------------------------------------------------------
	void ShowDebug()
	{
		if (!m_NearestTarget)
			return;

		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		vector muzzleFwd = barrelMat[2].Normalized();
		float dis = vector.Distance(barrelOrigin, m_NearestTarget.GetOrigin());
		Shape.CreateArrow(barrelOrigin, barrelOrigin + muzzleFwd * dis, 0.1, COLOR_BLUE, ShapeFlags.ONCE);

	}

	//------------------------------------------------------------------------------------------------
	void CheckCurrentTarget()
	{
		if (!IsEnemyAndAlive(m_NearestTarget) || !LineOfSightCheck(m_NearestTarget))
		{
			SetNewTarget(null);
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdate(float timeSlice)
	{
		if (!m_bActive)
			return;

		if (m_bDebug)
			ShowDebug();

		//Only Server checks for targets
		if (RplSession.Mode() != RplMode.Client)
		{
			m_fSearchDelay -= timeSlice;
			if (m_fSearchDelay <= 0)
			{
				if (!m_NearestTarget)
					CheckNextTarget();
				else
					CheckCurrentTarget();
				m_fSearchDelay = m_fMaxSearchDelay;
			}
		}

		Aim(timeSlice);

		m_fAttackSpeed -= timeSlice;
		if (m_fAttackSpeed <= 0 && m_bOnTarget)
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
		foreach (PointInfo muzzle : m_ProjectileMuzzles)
			muzzle.Init(owner);

		if (!GetGame().InPlayMode())
			return;

		SetAttackRange(m_iAttackRange);

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionAffiliation.GetAffiliatedFaction();

		m_AnimationController = AnimationControllerComponent.Cast(owner.FindComponent(AnimationControllerComponent));
		m_iShootCmd = m_AnimationController.BindCommand("CMD_SHOOT");

		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_iSignalBody = m_SignalsManager.AddOrFindSignal("BodyRotation", 0);
		m_iSignalBarrel = m_SignalsManager.AddOrFindSignal("BarrelRotation", 0);

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
		if (!m_bDebug)
			return;
		
		foreach (PointInfo muzzle : m_ProjectileMuzzles)
		{
			vector mat[4];
			muzzle.GetTransform(mat);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE | ShapeFlags.WIREFRAME, mat[3], 0.075);
		}
	}
}
