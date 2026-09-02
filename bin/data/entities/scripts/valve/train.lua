local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local SOURCE_TO_METERS = 0.07

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")

local speed = (tonumber(metadataValve["speed"]) or 200.0) * SOURCE_TO_METERS
local targetName = metadataValve["target"]

-- collect waypoints from path_corner entities chained via their target keyvalue
local waypoints = {}
local hasLoop = false
if type(targetName) == "string" and targetName ~= "" then
	local cornerByName = {}
	for i, e in ipairs(entities.all()) do
		if e:name() == "path_corner" then
			local meta = e:getComponent("Metadata")
			local valve = meta["valve"] or {}
			local tname = valve["targetname"]
			if tname then
				cornerByName[tname] = e
			end
		end
	end

	local current = cornerByName[targetName]
	local firstUid = nil
	local visited = {}
	while current and not visited[current:uid()] do
		visited[current:uid()] = true
		if firstUid == nil then firstUid = current:uid() end
		table.insert(waypoints, current:getComponent("Transform").position)

		local meta = current:getComponent("Metadata")
		local valve = meta["valve"] or {}
		current = cornerByName[valve["target"]]
	end
	hasLoop = (current ~= nil and current:uid() == firstUid)
end

-- 0 = idle, 1 = moving
local state = (#waypoints >= 2) and 1 or 0
local waypointIndex = 1
local lastPosition = transform.position

ent:bind( "tick", function(self)
	if state ~= 1 then return end

	local target = waypoints[waypointIndex]
	local toTarget = target - transform.position
	local dist = math.sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z)
	local step = time.delta() * speed

	if dist <= step then
		transform.position = target
		waypointIndex = waypointIndex + 1
		if waypointIndex > #waypoints then
			if hasLoop then
				waypointIndex = 1
			else
				state = 0
			end
		end
	else
		local dir = toTarget / dist
		transform.position = transform.position + dir * step
	end

	-- carry passengers riding on the train
	local delta = transform.position - lastPosition
	lastPosition = transform.position

	if physicsBody:initialized() then
		local box = physicsBody:bounds()
		for i, e in ipairs(entities.all()) do
			if e:uid() ~= ent:uid() then
				local body = e:getComponent("PhysicsBody")
				if body:initialized() then
					local pos = body:getTransform().position
					local riding =
						pos.x >= box.min.x - 0.6 and pos.x <= box.max.x + 0.6 and
						pos.z >= box.min.z - 0.6 and pos.z <= box.max.z + 0.6 and
						pos.y >= box.min.y - 0.5 and pos.y <= box.max.y + 2.5
					if riding then
						local t = e:getComponent("Transform")
						t.position = t.position + delta
					end
				end
			end
		end
	end
end )
