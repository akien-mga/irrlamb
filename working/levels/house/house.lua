-- Called when an orb is deactivated
function OnOrbDeactivate()
	UpdateState()
end

-- Progress the goal
function UpdateState()
	if State == 0 then
		GUI.TutorialText("Oh wait, almost forgot to shower first.", 8)
		ShowerSound = Audio.Play("shower.ogg", -6.019, 55.26, 45.16, 1, 0.0, 0.7, 8.0, 1.0)
		Level.CreateObject("orb", tOrb, -14.36, 64.94, 45.40, 0, 0, 0);
	elseif State == 1 then
		Audio.Stop(ShowerSound)
		GUI.TutorialText("Alright, enough splish splashing around. Let's brush some teeth.", 10)
		Level.CreateObject("orb", tOrb, -12.53, 65.70, 8.46, 0, 0, 0);
	elseif State == 2 then
		GUI.TutorialText("Oops, toilet needs to be flushed. Gross.", 15)
	elseif State == 3 then
		GUI.TutorialText("Breakfast time!", 7)
		Level.CreateObject("orb", tOrb, -35.75, 25.961304, 33.431503, 0, 0, 0);
	elseif State == 4 then
		GUI.TutorialText("Let's watch some sweet videos while we eat.", 15)
		Level.CreateObject("orb", tOrb, 41.8906, 7.574116, 40.508533, 0, 0, 0);
		X, Y, Z = Object.GetPosition(oSoccer);
		if Y > 3 then
			Object.SetLinearVelocity(oSoccer, 20, 0, 0);
		end
	elseif State == 5 then
		GUI.TutorialText("Ok, enough of that.", 5)
		Level.CreateObject("orb", tOrb, 39.859116, 8.510316, 6.891754, 0, 0, 0);
	elseif State == 6 then
		GUI.TutorialText("It's getting cold in here. Let's check on that furnace.", 10)
		Level.CreateObject("orb", tOrb, -37.7854, 22.8, -57.108673, 0, 0, 0);
	elseif State == 7 then
		GUI.TutorialText("You got sucked into the HVAC system. You almost died!", 12)
		Level.CreateObject("orb", tOrb, 97.503487, 77.7, -15.73, 0, 0, 0);
		FurnaceTeleport()
	end

	State = State + 1
end

-- Player collision handler
function OnHitPlayer(PlayerObject, OtherObject)

	-- Handle soccer ball riding
	if SoccerRode == 0 then
		if OtherObject == oSoccer then
			if SoccerStartTimer == 0 then
				SoccerStartTimer = Timer.Stamp()
			end
		elseif OtherObject == oHouse and SoccerStartTimer > 0 then
			SoccerRideTime = Timer.Stamp() - SoccerStartTimer
			SoccerRode = 1
			if SoccerRideTime > 2.0 and State >= 3 then
				GUI.TutorialText("You rode the ball for " .. string.format("%.3f", SoccerRideTime) .. " seconds. Nice!", 7)
			end
		end
	end
end

-- Zone handler
function OnHitZone(HitType, Zone, HitObject)

	-- Dismiss if exiting zone
	if HitType ~= 0 then
		return 0
	end

	-- Get object attributes
	ZoneName = Object.GetName(Zone)
	HitName = Object.GetName(HitObject)
	X, Y, Z = Object.GetPosition(Zone)

	-- Handle collision
	if HitObject == Player then
		if ZoneName == "zone_toilet_handle" then
			Audio.Play("flush.ogg", -8.84, 55.95, 31.62, 0, 0.0, 1.0, 10)
			if State == 3 then
				UpdateState()
			end
		elseif ZoneName == "zone_stairs" then
			if State < 4 then
				Object.Stop(Player)
				Object.SetPosition(Player, -9.255825, 60, -51.385429)
				GUI.TutorialText("You're not ready to go downstairs yet, doog!", 5)
			end
		elseif ZoneName == "zone_furnace" then
			if State >= 8 then
				FurnaceTeleport()
			end
		end
	elseif HitName == "salt" then
		if Spilt == 0 then
			GUI.TutorialText("Oh god you spilled the salt...", 5)
			Spilt = 1
		end
	end

	return 0
end

-- Teleport player to master bedroom
function FurnaceTeleport()
	Object.Stop(Player)
	Object.SetPosition(Player, 86.805458, 100, 36.694283)
end

-- Objects
tOrb = Level.GetTemplate("orb")
oSoccer = Object.GetPointer("soccer")
oHouse = Object.GetPointer("house")
ShowerSound = nil
Spilt = 0
SoccerStartTimer = 0
SoccerRode = 0

-- Sounds
Audio.Play("jazztown.ogg", 86, 72, 36, 1, 0.0, 1.0, 20.0, 20.0)
Audio.Play("furnace.ogg", -37, 11, -56, 1, 0.0, 1.0, 20.0, 10.0)

-- Show text
GUI.TutorialText("Wakey wakey! Time to get dressed for work!", 10)

-- Set up goal
State = 0
--UpdateState()
