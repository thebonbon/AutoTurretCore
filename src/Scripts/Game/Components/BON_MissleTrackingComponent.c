[ComponentEditorProps(category: "BON/Turrets", description: "Tracks Projectiles for cheaper distance calculation")]
class BON_ProjectileTrackingComponentClass : ScriptComponentClass
{
	static ref array<IEntity> s_aTrackedProjectiles = {};
}

class BON_ProjectileTrackingComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	void OnInventoryParentChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		Physics projectileRb = GetOwner().GetPhysics();
		if (!newSlot && projectileRb && projectileRb.IsActive())
			BON_ProjectileTrackingComponentClass.s_aTrackedProjectiles.Insert(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		InventoryItemComponent invComp = InventoryItemComponent.Cast(owner.FindComponent(InventoryItemComponent));
		if (invComp && invComp.m_OnParentSlotChangedInvoker)
			invComp.m_OnParentSlotChangedInvoker.Insert(OnInventoryParentChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (BON_ProjectileTrackingComponentClass.s_aTrackedProjectiles.Contains(owner))
			BON_ProjectileTrackingComponentClass.s_aTrackedProjectiles.RemoveItem(owner);
	}
}
