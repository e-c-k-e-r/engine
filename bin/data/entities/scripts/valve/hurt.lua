local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}
local physicsBody = ent:getComponent("PhysicsBody")

local damage = tonumber(metadataValve["damage"]) or 1.0
local delay = tonumber(metadataValve["delay"]) or 0.5

-- note: the engine has no health/damage system yet, so this trigger only
-- reports contact via I/O outputs (OnEntityTouch / OnTrigger / OnEndTouch)

local touching = {}

local function entityKey( body )
	local object = body:getObject()
	local meta = object:getComponent("Metadata")
	local valve = meta["valve"] or {}
	if valve["targetname"] then
		return tostring(valve["targetname"])
	end
	return tostring(object:uid())
end

ent:bind( "tick", function(self)
	if not physicsBody:initialized() then return end

	local currentCollisions = {}
	local collisionEvents = physicsBody:getCollisionEvents()

	for i, event in ipairs(collisionEvents) do
		local other = nil
		if event.a:getObject():uid() == ent:uid() then
			other = event.b
		elseif event.b:getObject():uid() == ent:uid() then
			other = event.a
		end

		if other then
			local key = entityKey(other)
			currentCollisions[key] = true

			if not touching[key] then
				touching[key] = true
				ent:queueHook("io:FireOutput.%UID%", { output = "OnEntityTouch", parameter = key }, 0)
				ent:queueHook("io:FireOutput.%UID%", { output = "OnTrigger" }, 0)
			end
		end
	end

	for key, _ in pairs(touching) do
		if not currentCollisions[key] then
			touching[key] = nil
			ent:queueHook("io:FireOutput.%UID%", { output = "OnEndTouch", parameter = key }, 0)
		end
	end
end )
