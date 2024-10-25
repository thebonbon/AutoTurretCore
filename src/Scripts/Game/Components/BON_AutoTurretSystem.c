class BON_AutoTurretSystem : GameSystem
{
	protected ref array<BON_AutoTurretComponent> m_aTurretManagers = {};

	//------------------------------------------------------------------------------------------------
	protected override void OnUpdate(ESystemPoint point)
	{
		float timeSlice;
		if (point == ESystemPoint.Frame)
			timeSlice = GetWorld().GetTimeSlice();
		else if (point == ESystemPoint.SimulatePhysics || point == ESystemPoint.PostSimulatePhysics)
			timeSlice = GetWorld().GetPhysicsTimeSlice();

		foreach (BON_AutoTurretComponent component : m_aTurretManagers)
		{
			if (component)
				component.OnUpdate(timeSlice);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDiag(float timeSlice)
	{
		DbgUI.Begin("BON_AutoTurretSystem");

		DbgUI.Text("Turrets active: " + m_aTurretManagers.Count());

		if (DbgUI.Button("Dump active BON_AutoTurretComponents"))
		{
			foreach (BON_AutoTurretComponent component : m_aTurretManagers)
			{
				Print(component.GetOwner(), LogLevel.ERROR);
			}
		}

		DbgUI.End();
	}

	//------------------------------------------------------------------------------------------------
	void Register(BON_AutoTurretComponent component)
	{
		if (component.GetOwner().IsDeleted() || (component.GetOwner().GetFlags() & EntityFlags.USER5))
			return;
		
		if (!m_aTurretManagers.Contains(component))
			m_aTurretManagers.Insert(component);
	}

	//------------------------------------------------------------------------------------------------
	void Unregister(BON_AutoTurretComponent component)
	{
		m_aTurretManagers.RemoveItem(component);
	}
}