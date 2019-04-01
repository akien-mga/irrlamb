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
		Level.Lose("You fell off!")
	else

		-- Check for active orbs
		HitObjectTemplate = Object.GetTemplate(HitObject)
		if HitObjectTemplate == tOrb then
			OrbState = Orb.GetState(HitObject)
			if OrbState == 0 then
				Level.Lose("You lost an orb!")
				return 0
			end
		end

		Object.SetLifetime(HitObject, 2)
	end

	return 0
end

-- Set up templates
tOrb = Level.GetTemplate("orb")
tConstraintFixed = Level.GetTemplate("constraint_fixed")
tConstraintX = Level.GetTemplate("constraint_x")
tConstraintZ = Level.GetTemplate("constraint_z")
tLogShort = Level.GetTemplate("log_short")
tLogMedium = Level.GetTemplate("log_medium")
tLogLong = Level.GetTemplate("log_long")

-- Grid parameters
Spacing = 7.1
Count = 5

-- Set up z logs
X = 0
Y = 0
Z = 0
for i = 1, Count do

	X = 0
	for j = 1, Count do
		oLog = Level.CreateObject("log_medium", tLogMedium, X, Y, Z, 0, 0, 0)
		Level.CreateConstraint("constraint", tConstraintX, oLog, nil)


		X = X + Spacing
	end

	Z = Z + Spacing
end

-- Set up x logs
Z = Spacing / 2
for i = 1, Count do
	X = -Spacing / 2
	for j = 1, Count do
		oLog = Level.CreateObject("log_medium", tLogMedium, X, Y, Z, 0, 90, 0)
		Level.CreateConstraint("constraint", tConstraintZ, oLog, nil)

		-- Create t log shape with orb
		if i == 2 and j == 3 then
			oLog1 = Level.CreateObject("log1", tLogShort, X, Y + 1.5, Z, 90, 0, 0)
			oOrb1 = Level.CreateObject("orb1", tOrb, X, Y - 1, Z, 0, 0, 0)
			Level.CreateConstraint("constraint", tConstraintFixed, oLog, oLog1)
			Level.CreateConstraint("constraint", tConstraintFixed, oLog, oOrb1)
		end

		-- Create z log with orb on top
		if i == 5 and j == 3 then
			oLog2 = Level.CreateObject("log2", tLogLong, X, Y + 1, Z - Spacing / 2, 0, 0, 0)
			oOrb2 = Level.CreateObject("orb2", tOrb, X, Y + 2, Z - Spacing / 2, 0, 0, 0)
			Level.CreateConstraint("constraint", tConstraintFixed, oLog2, oOrb2)
		end

		X = X + Spacing
	end

	Z = Z + Spacing
end

-- Set up goal
GoalCount = 5

