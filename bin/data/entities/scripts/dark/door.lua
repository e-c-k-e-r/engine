local ent = ent
local scene = entities.currentScene()

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local doorMeta = darkMeta["door"] or {}
local classTags = doorMeta["class_tags"] or (darkMeta["class_tags"] or "")

local state = 0

local isRotating = doorMeta["is_rotating"] == true
local closedPos = tonumber(doorMeta["closed"]) or 0.0
local openPos = tonumber(doorMeta["open"]) or 90.0
local speed = tonumber(doorMeta["speed"]) or 2.0
local axisRaw = tonumber(doorMeta["axis"]) or 2

local localAxis = Vector3f(0, 1, 0)
if axisRaw == 0 then
	localAxis = Vector3f(1, 0, 0)
elseif axisRaw == 1 then
	localAxis = Vector3f(0, 0, 1)
end

local currentWav = ""
local currentSchemaName = ""

local moveAxis = transform.orientation:rotate( localAxis )

if isRotating then
	closedPos = math.rad(closedPos)
	openPos = math.rad(openPos)
	speed = math.rad(speed)
else
	closedPos = closedPos * 0.7
	openPos = openPos * 0.7
end

local targetDistance = math.abs(openPos - closedPos)
local currentDistance = 0.0

local status = tonumber(doorMeta["status"]) or 0
if status == 1 then
	state = 2
	currentDistance = targetDistance
end

local polarity = (openPos > closedPos) and 1 or -1

local status = tonumber(doorMeta["status"]) or 0
local oldStateString = (status == 1) and "Open" or "Closed"

local function stopCurrentSound()
	if currentWav ~= "" then
		ent:callHook("sound:Stop.%UID%", { filename = currentWav })
		currentWav = ""
	end
end

local function playEnvSchema(newState)
	local queryTags = classTags
	if queryTags ~= "" then queryTags = queryTags .. ", " end
	queryTags = queryTags .. "Event StateChange, OpenState " .. newState .. ", OldOpenState " .. oldStateString

	local resolvedUrl, schemaName = _G.DarkUtils.playSound(ent, queryTags, nil, {
		spatial = true,
		volume = 1.0,
		maxDistance = 15.0,
		rolloffFactor = 1.0,
		unique = true
	})

	if resolvedUrl then
		if (newState == "Open" or newState == "Closed") and schemaName == currentSchemaName then
			return
		end
		stopCurrentSound()
		currentSchemaName = schemaName
		currentWav = resolvedUrl
	end
end


local function setDoorState(newState)
	if newState == "Open" and (state == 0 or state == 3) then
		oldStateString = (state == 0) and "Closed" or "Closing"
		state = 1
		playEnvSchema("Opening")

	elseif newState == "Close" and (state == 2 or state == 1) then
		oldStateString = (state == 2) and "Open" or "Opening"
		state = 3
		playEnvSchema("Closing")
	end
end
-- tick
ent:bind("tick", function(self)
	local deltaMove = 0
	local step = time.delta() * speed

	if state == 1 then
		deltaMove = step
		currentDistance = currentDistance + step

		if currentDistance >= targetDistance then
			deltaMove = deltaMove - (currentDistance - targetDistance)
			currentDistance = targetDistance
			state = 2

			ent:callHook("sound:Stop.%UID%", {})
			oldStateString = "Opening"
			playEnvSchema("Open")
		end

	elseif state == 3 then
		deltaMove = -step
		currentDistance = currentDistance - step

		if currentDistance <= 0 then
			deltaMove = deltaMove - currentDistance
			currentDistance = 0
			state = 0

			ent:callHook("sound:Stop.%UID%", {})
			oldStateString = "Closing"
			playEnvSchema("Closed")
		end
	end

	if deltaMove ~= 0 then
		local finalMove = deltaMove * polarity

		if isRotating then
			local rot = Quaternion.axisAngle(localAxis, finalMove)
			if physicsBody:initialized() then
				physicsBody:applyRotation(rot)
			else
				transform:rotate(rot)
			end
		else
			local vec = moveAxis * finalMove
			if physicsBody:initialized() then
				physicsBody:setVelocity(moveAxis * (finalMove / time.delta()))
			else
				transform.position = transform.position + vec
			end
		end
	else
		if not isRotating and physicsBody:initialized() then
			physicsBody:setVelocity(Vector3f(0, 0, 0))
		end
	end
end)

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message

	if msg == "TurnOn" then
		setDoorState("Open")
	elseif msg == "TurnOff" then
		setDoorState("Close")
	elseif msg == "Toggle" then
		if state == 0 or state == 3 then
			setDoorState("Open")
		else
			setDoorState("Close")
		end
	end
end)