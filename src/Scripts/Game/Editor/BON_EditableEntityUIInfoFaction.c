[BaseContainerProps()]
class BON_EditableEntityUIInfoFaction : SCR_EditableEntityUIInfo
{
	//------------------------------------------------------------------------------------------------
	//! Sets the ingame icon color to the faction color
	override bool SetIconTo(ImageWidget imageWidget)
	{
		Faction faction = GetFaction();
		if (faction)
			imageWidget.SetColor(faction.GetFactionColor());
		
		return super.SetIconTo(imageWidget);
		
	}
}
