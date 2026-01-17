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
	int m_iFactionID;

	//------------------------------------------------------------------------------------------------
	override void EOnPhysicsActive(IEntity owner, bool activeState)
	{
		if (m_TargetFlags == BON_TurretTargetFilterFlags.PROJECTILES)
			GetGame().GetAutoTurretGrid().Insert(owner, true, m_TargetFlags);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		//Set Faction for entities with FactionAffiliationComponent
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
		if (factionComp)
		{
			FactionManager factionManager = GetGame().GetFactionManager();
			m_iFactionID = factionManager.GetFactionIndex(factionComp.GetAffiliatedFaction());
		}

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
