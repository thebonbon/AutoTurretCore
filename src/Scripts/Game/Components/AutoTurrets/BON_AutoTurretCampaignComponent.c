[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretCampaignComponentClass : SCR_MilitaryBaseLogicComponentClass
{
}

class BON_AutoTurretCampaignComponent : SCR_MilitaryBaseLogicComponent
{
	FactionAffiliationComponent m_FactionComp;
	
	//------------------------------------------------------------------------------------------------
	//! Event which is triggered when the capturing faction changes
	override void OnCapturingFactionChanged(FactionKey faction)
	{
		super.OnCapturingFactionChanged();
		
		m_FactionComp.SetAffiliatedFactionByKey(faction);		
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_FactionComp = FactionAffiliationComponent.Cast(owner.FindComponent(FactionAffiliationComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		owner.SetEventMask(EntityEvent.INIT);
	}

}
