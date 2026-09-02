local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

local SOURCE_TO_METERS = 0.07

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")

local lip = (tonumber(metadataValve["lip"]) or 8.0) * SOURCE_TO_METERS
local wait = tonumber(metadataValve["wait"]) or 1.0
local flags = tonumber(metadataValve["spawnflags"]) or 0

-- movement direction: movedir vector, else angle keyvalue, else local up
local moveDir = Vector3f(0, 1, 0)
local movedir = metadataValve["movedir"]
if type(movedir) == "table" then
	moveDir = Vector3f(movedir[1], movedir[2], movedir[3]):normalize()
elseif type(movedir) == "string" and movedir ~= "" then
	local x, y, z = movedir:match("^%s*(-?%d+%.?%d*)%s+(-?%d+%.?%d*)%s+(-?%d+%.?%d*)%s*$")
	if x then
		moveDir = Vector3f(tonumber(x), tonumber(y), tonumber(z)):normalize()
	end
else
	local angle = tonumber(metadataValve["angle"])
	if angle then
		if angle == -1 then
			moveDir = Vector3f(0, 1, 0)
		elseif angle == -2 then
			moveDir = Vector3f(0, -1, 0)
		else
			local yaw = math.rad(angle)
			moveDir = Vector3f(math.cos(yaw), 0.0, -math.sin(yaw))
		end
	end
end

local pressTime = 0.5
if wait > 0 then
	pressTime = math.max(0.25, math.min(wait, 2.0))
end
local speed = lip / pressTime

-- 0 = idle, 1 = pressing out, 2 = held, 3 = retracting
local state = 0
local currentDistance = 0

local function press()
	if state ~= 0 then return end
	state = 1
	ent:queueHook("io:FireOutput.%UID%", { output = "OnPressed" }, 0)
	ent:queueHook("io:FireOutput.%UID%", { output = "OnUsed" }, 0)
end

local function release()
	if state == 2 then
		state = 3
	end
end

ent:bind( "tick", function(self)
	if state == 1 then
		local remaining = lip - currentDistance
		local move = math.min(time.delta() * speed, remaining)
		currentDistance = currentDistance + move
		transform.position = transform.position + moveDir * move

		if currentDistance >= lip then
			state = 2
			timer:reset()
		end
	elseif state == 3 then
		local move = math.min(time.delta() * speed, currentDistance)
		currentDistance = currentDistance - move
		transform.position = transform.position - moveDir * move

		if currentDistance <= 0 then
			state = 0
			ent:queueHook("io:FireOutput.%UID%", { output = "OnUnpressed" }, 0)
		end
	elseif state == 2 and wait > 0 then
		if timer:elapsed() >= wait then
			state = 3
		end
	end

	-- press when touched (floor buttons)
	if state == 0 and physicsBody:initialized() then
		local collisionEvents = physicsBody:getCollisionEvents()
		for i, event in ipairs(collisionEvents) do
			local other = nil
			if event.a:getObject():uid() == ent:uid() then
				other = event.b
			elseif event.b:getObject():uid() == ent:uid() then
				other = event.a
			end

			if other then
				press()
				break
			end
		end
	end
end )

ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user == ent:uid() then return end
	press()
end )

ent:addHook("io:Input.%UID%", function( payload )
	local input = payload.input

	if input == "Press" or input == "Down" then
		press()
	elseif input == "Release" or input == "Up" or input == "Reset" then
		release()
	end
end)
