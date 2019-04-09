
-- Called when an orb is deactivated
function OnOrbDeactivate()
	GoalCount = GoalCount - 1
	if GoalCount == 0 then
		Level.Win()
	end
end

-- Display lose message
function OnHitZone(HitType, Zone, HitObject)
	if HitObject == Player then
		Level.Lose("You crashed!")
	else
		Object.SetLifetime(HitObject, 2)
	end

	return 0
end

-- Set up goal
GoalCount = 5
Camera.SetPitch(89)
