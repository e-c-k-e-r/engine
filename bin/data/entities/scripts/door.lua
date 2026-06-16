local ent = ent
local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

local state = 0
local currentDistance = 0
local polarity = 1

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local metadata = ent:getComponent("Metadata")
local metadataDoor = metadata["door"] or {}
local metadataValve = metadata["valve"] or {}

local speed = metadataDoor["speed"] or 100.0
local wait = metadataDoor["wait"] or 4.0
local isRotating = (metadataDoor["axis"] ~= nil)

local targetDistance = 0
local moveDir = Vector3f(0, 0, 0)
local rotAxis = Vector3f(0, 1, 0)
local flags = metadataDoor["spawnflags"] or 0

local isToggle = (math.floor(flags / 32) % 2) ~= 0
if isToggle then
	wait = -1
end

local sounds = {
	locked = "doors/default_locked.wav",
	open = "doors/default_move.wav",
	close = "doors/default_stop.wav",
}

if metadataValve["noise1"] then
	sounds["open"] = metadataValve["noise1"]
end
if metadataValve["noise2"] then
	sounds["close"] = metadataValve["noise2"]
end

if isRotating then
	local ax = metadataDoor["axis"]
	rotAxis = Vector3f(ax[1], ax[2], ax[3]):normalize()

	local dist = metadataDoor["distance"] or 90.0

	local isReverse = (math.floor(flags / 2) % 2) ~= 0
	if isReverse then
		polarity = -1
	end

	polarity = polarity * -1

	if dist < 0 then
		polarity = polarity * -1
		dist = math.abs(dist)
	end

	targetDistance = math.rad(dist)
	speed = math.rad(speed)
else
	local dir = metadataDoor["direction"]
	if dir then moveDir = Vector3f(dir[1], dir[2], dir[3]):normalize() end

	local lip = metadataDoor["lip"] or 8.0

	if physicsBody:initialized() then
		local obb = OBB(physicsBody:bounds())
		local size = obb.extent * 2.0

		local travelSize = math.abs(moveDir:dot(size))

		if travelSize < 0.8 then
			travelSize = 2.4
		end

		targetDistance = travelSize - lip
	else
		targetDistance = 2.4 - lip
	end

	if targetDistance <= 0 then targetDistance = 0.1 end
end

local soundEmitter = ent

local playSound = function( key, loop )
	if not loop then loop = false end
	local url = "valve://sound/" .. key
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true, streamed = true, volume = "sfx", loop = loop
	}, 0)
end

local function toggleDoor( payload )
	if state == 0 or state == 3 then
		state = 1
		playSound(sounds["open"])

	   if isRotating and payload.uid ~= nil then
			local isOneWay = (math.floor(flags / 16) % 2) ~= 0

			if not isOneWay then
				local user = entities.get( payload.user )
				if user and physicsBody:initialized() then
					local userPos = user:getComponent("Transform").position

					local obb = OBB(physicsBody:bounds())
					local doorCenter = obb.center
					local hingePos = transform.position

					local doorBlade = doorCenter - hingePos
					local doorNormal = rotAxis:cross(doorBlade):normalize()

					local delta = userPos - doorCenter

					if doorNormal:dot(delta) > 0 then
						polarity = 1
					else
						polarity = -1
					end

					local baseDist = metadataDoor["distance"] or 90.0
					if baseDist < 0 then
						polarity = polarity * -1
					end
				end
			end
		end
	elseif state == 2 then
		state = 3
		playSound(sounds["open"])
	end
end

-- on tick
ent:bind( "tick", function(self)
	local deltaMove = 0
	local step = time.delta() * speed

	if state == 1 then
		deltaMove = step
		currentDistance = currentDistance + step

		if currentDistance >= targetDistance then
			deltaMove = deltaMove - (currentDistance - targetDistance)
			currentDistance = targetDistance
			state = 2
			timer:reset()
			playSound(sounds["close"])

			ent:queueHook("io:FireOutput.%UID%", { output = "OnOpen" }, 0)
		end
	elseif state == 3 then
		deltaMove = -step
		currentDistance = currentDistance - step

		if currentDistance <= 0 then
			deltaMove = deltaMove - currentDistance
			currentDistance = 0
			state = 0
			playSound(sounds["close"])
		end
	elseif state == 2 and wait >= 0 then
		if timer:elapsed() >= wait then
			state = 3
			playSound(sounds["open"])
		end
	end

	if deltaMove ~= 0 then
		local finalMove = deltaMove * polarity

		if isRotating then
			local rot = Quaternion.axisAngle(rotAxis, finalMove)
			if physicsBody:initialized() then
				physicsBody:applyRotation(rot)
			else
				transform:rotate(rot)
			end
		else
			local vec = moveDir * finalMove
			if physicsBody:initialized() then
				physicsBody:setVelocity(moveDir * (finalMove / time.delta()))
			else
				transform.position = transform.position + vec
			end
		end
	else
		if not isRotating and physicsBody:initialized() then
			physicsBody:setVelocity(Vector3f(0,0,0))
		end
	end
end )

-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user == ent:uid() then return end

	local useOpens = (math.floor(flags / 256) % 2) ~= 0

	if useOpens then
		toggleDoor( payload )

		ent:queueHook("io:FireOutput.%UID%", { output = "OnUse" }, 0)
	else
		playSound(sounds["locked"])
	end
end )

ent:addHook("io:Input.%UID%", function( payload )
	local input = payload.input
	print(ent:name() .. " received I/O Input: " .. input)

	local mockPayload = {
		user = payload.caller,
		uid = ent:uid()
	}

	if input == "Open" and (state == 0 or state == 3) then
		toggleDoor(mockPayload)
	elseif input == "Close" and (state == 2 or state == 1) then
		toggleDoor(mockPayload)
	elseif input == "Toggle" or input == "Use" then
		toggleDoor(mockPayload)
	end
end)