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

		requires.Insert(FactionAffiliationComponent);
		requires.Insert(SignalsManagerComponent);
		requires.Insert(BaseItemAnimationComponent);

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

	[Attribute("w_body", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBodyBone;

	[Attribute("w_barrel", UIWidgets.Auto, "", category: "Setup")]
	protected string m_sBarrelBone;

	[Attribute("-180 180 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitHorizontal;

	[Attribute("-25 85 0", UIWidgets.Auto, desc: "x = min, y = max, ignore z", category: "Setup")]
	protected vector m_vLimitVertical;

	[Attribute(uiwidget: UIWidgets.Flags, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	BON_TurretTargetFilterFlags m_eTargetFlags;

	[Attribute("0", UIWidgets.CheckBox, "Trigger projectile near target?", category: "Setup")]
	bool m_bTriggerOnTarget;

	//--- AIMING ---
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

	[Attribute("0", uiwidget: UIWidgets.ComboBox, "Direct: Aim straight at the target's current position | Guided: Follow the target (missiles only!) | Intercept: Lead the target to intercept at predicted position", enums: ParamEnumArray.FromEnum(BON_TurretFireMode), category: "Aiming")]
	BON_TurretFireMode m_eFireMode;

	//--- MISC ---
	[Attribute("0", UIWidgets.CheckBox, "Enable debug?", category: "Debug")]
	bool m_bDebug;

	[RplProp(onRplName: "OnTargetChanged")]
	RplId m_iNearestTargetId;


	protected float m_fAttackSpeed = 0;
	protected float m_fMaxSearchTime = 0.1;
	protected float m_fSearchTimer = 0;
	protected int m_iCurrentMuzzle;
	bool m_bActive = false;

	protected ref Faction m_Faction;
	protected AnimationControllerComponent m_AnimationController;
	protected PerceivableComponent m_TargetPerceivableComp;
	protected SignalsManagerComponent m_SignalsManager;

	protected int m_iSignalBody;
	protected int m_iSignalBarrel;
	protected int m_iShootCmd;
	protected int m_iBodyVar;
	protected TNodeId m_iBarrelBoneIndex;
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
	protected IEntity m_Target;
	ref Shape m_LoSDebug;

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
	//! Server + Client
	void Aim(float timeSlice)
	{
		m_fLerp += timeSlice * m_fRotationSpeed;
		m_fLerp = Math.Clamp(m_fLerp, 0, 1);

		//Back to rest position
		if (!m_Target)
		{
			m_bOnTarget = false;

			m_fNewBodyYaw = Math.Lerp(m_fCurrentBodyYaw, 0, m_fLerp);
			m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, 0, m_fLerp);

			m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);
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
		vector targetAimPoint = m_Target.GetOrigin();
		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		//Add Aimpoint
		if (m_TargetPerceivableComp)
		{
			array<vector> aimPoints();
			m_TargetPerceivableComp.GetAimpoints(aimPoints);
			if (!aimPoints.IsEmpty())
				targetAimPoint = aimPoints[0];
		}

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

		vector muzzleFwd = barrelMat[2].Normalized();
		vector dir = vector.Direction(GetOwner().GetOrigin(), targetAimPoint);

		Shape.CreateArrow(GetOwner().GetOrigin(), GetOwner().GetOrigin() + dir, 0.1, COLOR_GREEN, ShapeFlags.ONCE);
		Shape.CreateArrow(GetOwner().GetOrigin(), GetOwner().GetOrigin() + muzzleFwd * 10, 0.1, COLOR_YELLOW, ShapeFlags.ONCE);
		Shape.CreateSphere(COLOR_RED, ShapeFlags.ONCE | ShapeFlags.NOZWRITE, targetAimPoint, 0.25);

		//Set Signals
		vector targetDir = targetAimPoint - barrelOrigin;
		vector localTargetDir = GetOwner().VectorToLocal(targetDir);
		vector angles = SCR_Math3D.FixEulerVector180(localTargetDir.VectorToAngles());

		m_fNewBodyYaw = Math.Lerp(m_fCurrentBodyYaw, -angles[0], m_fLerp);
		m_fNewBarrelPitch = Math.Lerp(m_fCurrentBarrelPitch, angles[1], m_fLerp);

		m_SignalsManager.SetSignalValue(m_iSignalBody, m_fNewBodyYaw);
		m_SignalsManager.SetSignalValue(m_iSignalBarrel, m_fNewBarrelPitch);

		if (m_fLerp == 1)
		{
			m_bOnTarget = true;
			m_fLerp = 0;
			m_fCurrentBodyYaw = m_fNewBodyYaw;
			m_fCurrentBarrelPitch = m_fNewBarrelPitch;
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsEnemyAndAlive(IEntity ent)
	{
		#ifdef WORKBENCH
		if (ent.GetName() == "BOB")
			return true;
		#endif

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
	void LaunchProjectile(notnull IEntity projectile)
	{
		if (m_bTriggerOnTarget)
		{
			float timeToTarget;
			float targetDistance = vector.Distance(GetOwner().GetOrigin(), m_Target.GetOrigin());
			BallisticTable.GetHeightFromProjectile(targetDistance, timeToTarget, projectile);

			if (timeToTarget > 0)
			{
				timeToTarget += s_AIRandomGenerator.RandFloatXY(-0.2, 0.2);
				GetGame().GetCallqueue().CallLater(TriggerProjectile, timeToTarget * 1000, false, projectile); //SetTimer on TimerTriggerComponent does not work :(
			}
		}

		BON_GuidedProjectile guidedProjectile = BON_GuidedProjectile.Cast(projectile);
		if (guidedProjectile)
		{
			if (m_fRocketGuidanceStrength != 0)
				guidedProjectile.m_fGuidanceStrength = m_fRocketGuidanceStrength;
			guidedProjectile.SetTargetAndLaunch(m_Target, m_eFireMode);
			return;
		}

		ProjectileMoveComponent moveComp = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
		if (moveComp)
			moveComp.Launch(projectile.GetTransformAxis(2).Normalized(), vector.Zero, 1, projectile, GetOwner(), null, null, null);

		if (m_AnimationController)
			m_AnimationController.CallCommand(m_iShootCmd, 1, 0);
	}

	//------------------------------------------------------------------------------------------------
	void Fire()
	{
		if (!m_Target)
			return;

		vector muzzleMat[4];
		m_ProjectileMuzzles[m_iCurrentMuzzle].GetTransform(muzzleMat);
		vector inaccuracyAngles = Vector(
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy),
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy),
				s_AIRandomGenerator.RandFloatXY(-m_fAttackInaccuracy, m_fAttackInaccuracy)
		);
		vector inaccuracyMat[3];
		Math3D.AnglesToMatrix(inaccuracyAngles, inaccuracyMat);
		Math3D.MatrixMultiply3(muzzleMat, inaccuracyMat, muzzleMat);

		EntitySpawnParams spawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = muzzleMat;

		if (!m_bIsProjectileReplicated || Replication.IsServer())
		{
			IEntity lastSpawnedProjectile = GetGame().SpawnEntityPrefab(Resource.Load(m_Projectile), GetGame().GetWorld(), spawnParams);
			if (!lastSpawnedProjectile)
				return;

			LaunchProjectile(lastSpawnedProjectile);
		}

		SoundComponent soundComponent = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));
		if (soundComponent)
			soundComponent.SoundEvent(m_sShootSound);

		if (m_sMuzzleParticle)
		{
			ParticleEffectEntitySpawnParams params();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform = muzzleMat;
			ParticleEffectEntity particleEmitter = ParticleEffectEntity.SpawnParticleEffect(m_sMuzzleParticle, params);
		}

		bool triggerProjectile = s_AIRandomGenerator.RandIntInclusive(1, 100) < m_fProjectileTriggerChance;
		if (m_Target.FindComponent(BaseTriggerComponent) && triggerProjectile)
			TriggerProjectile(m_Target);

		m_iCurrentMuzzle++;
		if (m_iCurrentMuzzle >= m_ProjectileMuzzles.Count())
			m_iCurrentMuzzle = 0;
	}

	//------------------------------------------------------------------------------------------------
	void ShowDebug()
	{
		Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.ONCE | ShapeFlags.WIREFRAME, GetOwner().GetOrigin(), m_iAttackRange);

		array<ref BON_AutoTurretTarget> sortedTargets = {};
		GetGame().GetAutoTurretGrid().FindSortedTargetsInRage(GetOwner(), sortedTargets, GetOwner().GetOrigin(), m_iAttackRange, m_eTargetFlags);

		foreach (BON_AutoTurretTarget target : sortedTargets)
		{
			Shape.CreateSphere(COLOR_BLUE, ShapeFlags.ONCE | ShapeFlags.WIREFRAME | ShapeFlags.NOZBUFFER, target.m_Ent.GetOrigin(), 0.2);
		}

		if (!m_Target)
			return;

		vector barrelMat[4];
		GetOwner().GetAnimation().GetBoneMatrix(m_iBarrelBoneIndex, barrelMat);
		vector barrelOrigin = GetOwner().CoordToParent(barrelMat[3]);

		vector muzzleFwd = barrelMat[2].Normalized();
		float dis = vector.Distance(barrelOrigin, m_Target.GetOrigin());
		Shape.CreateArrow(barrelOrigin, barrelOrigin + muzzleFwd * dis, 0.1, COLOR_BLUE, ShapeFlags.ONCE);

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
	}

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
	//! Gets closest target with line of sight
	IEntity FindTarget()
	{
		array<ref BON_AutoTurretTarget> sortedTargets = {};
		GetGame().GetAutoTurretGrid().FindSortedTargetsInRage(GetOwner(), sortedTargets, GetOwner().GetOrigin(), m_iAttackRange, m_eTargetFlags);

		foreach (BON_AutoTurretTarget target : sortedTargets)
		{
			//In Aim limits
			vector dirToTarget = (target.m_Ent.GetOrigin() - GetOwner().GetOrigin()).Normalized();
			vector angles = SCR_Math3D.FixEulerVector180(dirToTarget.VectorToAngles());


			bool inHorizontal = angles[1] >= m_vLimitHorizontal[0] && angles[1] <= m_vLimitHorizontal[1];
			bool inVertical = -angles[0] >= m_vLimitVertical[0] && angles[0] <= m_vLimitVertical[1];
			Print(inHorizontal && inVertical);
			if (LineOfSightTo(target.m_Ent) && IsEnemyAndAlive(target.m_Ent))
				return target.m_Ent;
			else
				Print("no los or enemy");
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	bool LineOfSightTo(IEntity ent)
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
			if (!aimPoints.IsEmpty())
				targetAimPoint = aimPoints[0];
		}

		TraceParam param = new TraceParam();
		param.Start = muzzleMat[3];
		param.End = targetAimPoint;
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.VISIBILITY;
		param.Exclude = GetOwner();
		param.LayerMask = EPhysicsLayerPresets.Projectile;
		float traceDistance = GetOwner().GetWorld().TraceMove(param, null);

		if (m_bDebug)
		{
			vector position = (param.End - param.Start) * traceDistance + param.Start;
			m_LoSDebug = Shape.CreateArrow(muzzleMat[3], position, 0.1, COLOR_GREEN, ShapeFlags.NOZBUFFER);
		}

		bool canHit = (traceDistance == 1) || (param.TraceEnt == ent);
		if (canHit)
			return true;

		if (!param.TraceEnt)
			return false;
	}

	//------------------------------------------------------------------------------------------------
	bool CheckCurrentTarget()
	{
		//Is target still a target?
		DamageManagerComponent dmgManager = DamageManagerComponent.Cast(m_Target.FindComponent(DamageManagerComponent));
		if (dmgManager && dmgManager.IsDestroyed())
			return false;

		//Simple distance check first
		float targetDist = vector.Distance(GetOwner().GetOrigin(), m_Target.GetOrigin());
		if (targetDist > m_iAttackRange)
			return false;

		//If still in range raycast
		bool canHit = LineOfSightTo(m_Target);
		return canHit;
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdateServer(float timeSlice)
	{
		m_fSearchTimer -= timeSlice;
		if (m_fSearchTimer > 0)
			return;

		m_fSearchTimer = m_fMaxSearchTime;

		//No or invalid target -> Find new one
		if (!m_Target || !CheckCurrentTarget())
			SetNewTarget(FindTarget());
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdateClient(float timeSlice)
	{
		if (m_bDebug)
			ShowDebug();

		Aim(timeSlice);

		m_fAttackSpeed -= timeSlice;
		if (m_fAttackSpeed <= 0 && m_bOnTarget)
		{
			Fire();
			m_fAttackSpeed = m_fMaxAttackSpeed;
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnUpdate(float timeSlice)
	{
		if (!m_bActive)
			return;

		if (Replication.IsServer())
			OnUpdateServer(timeSlice);

		if (RplSession.Mode() != RplMode.Dedicated)
			OnUpdateClient(timeSlice);
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

		FactionAffiliationComponent factionAffiliation = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		m_Faction = factionAffiliation.GetAffiliatedFaction();

		m_AnimationController = BaseItemAnimationComponent.Cast(owner.FindComponent(BaseItemAnimationComponent));
		m_iShootCmd = m_AnimationController.BindCommand("CMD_SHOOT");
		m_iBarrelBoneIndex = GetOwner().GetAnimation().GetBoneIndex(m_sBarrelBone);

		m_SignalsManager = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
		m_iSignalBody = m_SignalsManager.AddOrFindSignal("BodyRotation", 0);
		m_iSignalBarrel = m_SignalsManager.AddOrFindSignal("BarrelRotation", 0);

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

		foreach (PointInfo muzzle : m_ProjectileMuzzles)
		{
			vector mat[4];
			muzzle.GetTransform(mat);
			Shape.CreateSphere(Color.RED, ShapeFlags.ONCE | ShapeFlags.WIREFRAME, mat[3], 0.075);
		}
	}
}
