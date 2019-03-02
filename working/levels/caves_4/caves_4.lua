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
		Level.Lose()
	else
		Object.SetLifetime(HitObject, 2)
	end

	return 0
end

-- Display lose message
function OnHitPlayer(PlayerObject, OtherObject)

	if OtherObject == oIce then
		Level.Lose("You fell off the raft!")
	else
		Template = Object.GetTemplate(OtherObject)
		if Template == tOrb or Template == tRaft then
			Object.SetLifetime(OtherObject, 5)
		end
	end
end

-- Set up goal
GoalCount = 5

-- Get static collision object
oIce = Object.GetPointer("ice")
tOrb = Level.GetTemplate("orb")
tRaft = Level.GetTemplate("raft")

-- Set up camera
Camera.SetYaw(0)
--Camera.SetPitch(0)
