[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetComponent : ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	BON_TurretTargetFilterFlags m_TargetFlags;
	
	//------------------------------------------------------------------------------------------------
	override void EOnPhysicsActive(IEntity owner, bool activeState)
	{
		if (m_TargetFlags != BON_TurretTargetFilterFlags.PROJECTILES)
			return;
		
		if (activeState)
			BON_AutoTurretTargets.s_aTargetProjectiles.Insert(GetOwner());
		else
			BON_AutoTurretTargets.s_aTargetProjectiles.RemoveItem(GetOwner());
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		switch (m_TargetFlags)
		{
			case BON_TurretTargetFilterFlags.VEHICLES:
				BON_AutoTurretTargets.s_aTargetVehicles.Insert(owner);
				break;
			case BON_TurretTargetFilterFlags.AIRCRAFTS:
				BON_AutoTurretTargets.s_aTargetAircrafts.Insert(owner);
				break;
			case BON_TurretTargetFilterFlags.CHARACTERS:
				BON_AutoTurretTargets.s_aTargetCharacters.Insert(owner);
				break;
		}
		
		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent("SOUND_TARGET_BEEP");
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.PHYSICSACTIVE);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		switch (m_TargetFlags)
		{
			case BON_TurretTargetFilterFlags.VEHICLES:
				BON_AutoTurretTargets.s_aTargetVehicles.RemoveItem(owner);
				break;
			case BON_TurretTargetFilterFlags.AIRCRAFTS:
				BON_AutoTurretTargets.s_aTargetAircrafts.RemoveItem(owner);
				break;
			case BON_TurretTargetFilterFlags.CHARACTERS:
				BON_AutoTurretTargets.s_aTargetCharacters.RemoveItem(owner);
				break;
		}
	}
}
