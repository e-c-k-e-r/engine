local ent = ent
local scene = entities.currentScene()

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}

local speed = 2.0
local state = 0
local currentTargetPos = nil
local activeWaypointDarkID = nil

local function findInitialWaypoint()
	local conns = darkMeta["connections"] or {}
	for _, conn in ipairs(conns) do
		if conn.flavor == "TPathInit" or conn.flavor == "TPathNext" or conn.flavor == "TPath" then
			return conn.target_id
		end
	end
	return nil
end

local currentWaypointID = findInitialWaypoint()

local function getWaypointPos(darkID)
	if not darkID then return nil end
	local targetUID = _G.DarkTargets[darkID]
	if targetUID then
		local wpEnt = entities.get(targetUID)
		if wpEnt then
			return wpEnt:getComponent("Transform").position, targetUID
		end
	end
	return nil
end

local function advanceWaypoint(darkID)
	local targetUID = _G.DarkTargets[darkID]
	if targetUID then
		local wpEnt = entities.get(targetUID)
		if wpEnt then
			local wpMeta = wpEnt:getComponent("Metadata")["dark"] or {}
			local conns = wpMeta["connections"] or {}
			for _, conn in ipairs(conns) do
				if conn.flavor == "TPathNext" or conn.flavor == "TPath" then
					return conn.target_id
				end
			end
		end
	end
	return nil
end

ent:bind("tick", function(self)
	if state == 1 and currentTargetPos then
		local myPos = transform.position
		local dir = currentTargetPos - myPos

		local dist = dir:norm()
		local step = time.delta() * speed

		if dist <= step then
			if physicsBody:initialized() then
				physicsBody:setVelocity(Vector3f(0, 0, 0))
			end
			transform.position = currentTargetPos
			state = 0

			local nextWp = advanceWaypoint(activeWaypointDarkID)
			if nextWp then
				currentWaypointID = nextWp
			end

		else
			local vel = dir:normalize() * speed
			if physicsBody:initialized() then
				physicsBody:setVelocity(vel)
			else
				transform.position = myPos + (vel * time.delta())
			end
		end
	end
end)

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message


	if (msg == "TurnOn" or msg == "TurnOff") and state == 0 then
		local targetPos, tUid = getWaypointPos(currentWaypointID)

		if targetPos and (transform.position - targetPos):norm() < 0.1 then
			local nextWp = advanceWaypoint(currentWaypointID)
			if nextWp then
				currentWaypointID = nextWp
				targetPos, tUid = getWaypointPos(currentWaypointID)
			end
		end

		if targetPos then
			currentTargetPos = targetPos
			activeWaypointDarkID = currentWaypointID
			state = 1
		end
	end
end)