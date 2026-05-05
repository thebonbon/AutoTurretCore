class BON_AutoTurretBaseLogicComponentClass : SCR_MilitaryBaseLogicComponentClass
{
}

class BON_AutoTurretBaseLogicComponent : SCR_MilitaryBaseLogicComponent
{
	override void OnBaseFactionChanged(Faction faction)
	{
		super.OnBaseFactionChanged(faction);
		if (faction)
			Print("[ATC] Turret changed faction to " + faction.GetFactionKey());
	}
}
