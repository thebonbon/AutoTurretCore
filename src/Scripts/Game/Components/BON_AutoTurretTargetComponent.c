[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetComponent : ScriptComponent
{
	[Attribute(uiwidget: UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(BON_TurretTargetFilterFlags), category: "Setup")]
	BON_TurretTargetFilterFlags m_TargetFlags;
	
	private Physics m_Rb;
	
	//------------------------------------------------------------------------------------------------
	void OnInventoryParentChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		if (!newSlot && m_Rb && m_Rb.IsActive())
			BON_AutoTurretTargets.s_aTargetProjectiles.Insert(GetOwner());
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
			case BON_TurretTargetFilterFlags.PROJECTILES:
			{
				m_Rb = GetOwner().GetPhysics();
				InventoryItemComponent invComp = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
				if (invComp && invComp.m_OnParentSlotChangedInvoker)
					invComp.m_OnParentSlotChangedInvoker.Insert(OnInventoryParentChanged);			
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
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
			case BON_TurretTargetFilterFlags.PROJECTILES:
			{
				InventoryItemComponent invComp = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
				if (invComp && invComp.m_OnParentSlotChangedInvoker)
					invComp.m_OnParentSlotChangedInvoker.Remove(OnInventoryParentChanged);		
				BON_AutoTurretTargets.s_aTargetCharacters.RemoveItem(owner);	
				break;
			}
		}
	}
}
