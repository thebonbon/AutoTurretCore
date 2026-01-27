enum BON_TurretTargetFilterFlags
{
	CHARACTERS = 1 << 1,
	VEHICLES = 1 << 2,
	AIRCRAFTS = 1 << 3,
	PROJECTILES = 1 << 4,
}

[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, enumType: BON_TurretTargetFilterFlags, category: "Setup")]
	BON_TurretTargetFilterFlags m_TargetFlags;

	//Used for e.g missile IFF (Instigator faction)
	int m_iFactionID = -1;
	protected SoundComponent m_SoundComp;
	protected AudioHandle m_AlarmAudioHandle = AudioHandle.Invalid;
	protected int m_iActiveInstances;

	static const int PITCH_TARGET_SPOTTED = -6;
	static const int PITCH_SHOOTING = 6;

	//------------------------------------------------------------------------------------------------
	void StopAlarm()
	{
		if (!m_SoundComp)
			return;

		m_iActiveInstances--;

		if (m_iActiveInstances <= 0)
		{
			m_SoundComp.Terminate(m_AlarmAudioHandle);
			m_AlarmAudioHandle = AudioHandle.Invalid;
			m_iActiveInstances = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Surley there is a better way to do this but oh well ¯\_(ツ)_/¯
	//! Counts active instances to not stop if multiple turrets target this ent but only one looses contact
	void SetAlarm(int pitch)
	{
		if (!m_SoundComp)
			return;

		//New target created
		if (pitch == BON_AutoTurretTargetComponent.PITCH_TARGET_SPOTTED)
			m_iActiveInstances++;

		m_SoundComp.SetSignalValueStr("AutoTurretAlarmPitch", pitch);
		
		//First time this ent is a target
		if (m_AlarmAudioHandle == AudioHandle.Invalid)
			m_AlarmAudioHandle = m_SoundComp.SoundEvent("SOUND_ATC_WARN");
	}

	//------------------------------------------------------------------------------------------------
	override void EOnPhysicsActive(IEntity owner, bool activeState)
	{
		if (m_TargetFlags == BON_TurretTargetFilterFlags.PROJECTILES)
			GetGame().GetAutoTurretGrid().Insert(owner, true, m_TargetFlags);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_SoundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		GetGame().GetAutoTurretGrid().Insert(owner, true, m_TargetFlags);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	//! On Death remove self as target
	override void EOnDeactivate(IEntity owner)
	{
		GetGame().GetAutoTurretGrid().Remove(owner);
	}
}

modded class ArmaReforgerScripted
{
	protected ref BON_AutoTurretGridMap m_AutoTurretGridMap;

	//------------------------------------------------------------------------------------------------
	BON_AutoTurretGridMap GetAutoTurretGrid()
	{
		if (!m_AutoTurretGridMap)
			m_AutoTurretGridMap = new BON_AutoTurretGridMap();

		return m_AutoTurretGridMap;
	}
}
