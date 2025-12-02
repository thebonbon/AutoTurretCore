[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class BON_AutoTurretTargetingComponentClass : ScriptComponentClass
{
}

class BON_AutoTurretTargetingComponent : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		// SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}
}
