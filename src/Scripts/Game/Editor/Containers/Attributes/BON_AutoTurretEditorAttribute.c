[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretTargetsEditorAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		super.ReadVariable(item, manager);

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		array<int> enumValues = {};
		SCR_Enum.GetEnumValues(BON_TurretTargetFilterFlags, enumValues);
		foreach (int value : enumValues)
		{
			AddOrderedState(SCR_Enum.HasPartialFlag(autoTurretComponent.m_TargetFlags, value));
		}

		return SCR_BaseEditorAttributeVar.CreateVector(GetFlagVector());
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		super.WriteVariable(item, var, manager, playerID);

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return;

		array<int> enumValues = {};
		SCR_Enum.GetEnumValues(BON_TurretTargetFilterFlags, enumValues);
		foreach (int value : enumValues)
		{
			if (GetOrderedState())
				autoTurretComponent.m_TargetFlags = SCR_Enum.SetFlag(autoTurretComponent.m_TargetFlags, value);
			else
				autoTurretComponent.m_TargetFlags = SCR_Enum.RemoveFlag(autoTurretComponent.m_TargetFlags, value);
		}
	}

	//------------------------------------------------------------------------------------------------
	override protected void CreatePresets()
	{
		m_aValues.Clear();

		array<int> enumValues = {};
		SCR_Enum.GetEnumValues(BON_TurretTargetFilterFlags, enumValues);
		foreach (int value : enumValues)
		{
			SCR_EditorAttributeFloatStringValueHolder valueHolder = new SCR_EditorAttributeFloatStringValueHolder();
			valueHolder.SetName(typename.EnumToString(BON_TurretTargetFilterFlags, value));
			m_aValues.Insert(valueHolder);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretEnabledEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(autoTurretComponent.m_bActive);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_bActive = var.GetBool();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretRangeEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(autoTurretComponent.GetAttackRange());
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.SetAttackRange(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretInaccuracyEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(autoTurretComponent.m_fAttackInaccuracy);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_fAttackInaccuracy = var.GetFloat();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretAttackSpeedEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(autoTurretComponent.m_fMaxAttackSpeed);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_fMaxAttackSpeed = var.GetFloat();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretRotationSpeedEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(autoTurretComponent.m_fRotationSpeed);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_fRotationSpeed = var.GetFloat();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretRocketGuidanceEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(autoTurretComponent.m_fRocketGuidanceStrength);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_fRocketGuidanceStrength = var.GetFloat();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class BON_AutoTurretRocketTriggerChanceEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (!autoTurretComponent)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(autoTurretComponent.m_fProjectileTriggerChance);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BON_AutoTurretComponent autoTurretComponent = BON_AutoTurretComponentClass.IsAutoTurret(item);
		if (autoTurretComponent)
			autoTurretComponent.m_fProjectileTriggerChance = var.GetFloat();
	}
}
