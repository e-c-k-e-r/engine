local ent = ent
local scene = entities.currentScene()
local physicsBody = ent:getComponent("PhysicsBody")
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}

local touching = {}

local tripFlags = darkMeta["trip_flags"] or 0
local triggerOnEnter = bit.band(tripFlags, 1) ~= 0 or tripFlags == 0
local triggerOnExit  = bit.band(tripFlags, 2) ~= 0
local triggerOnce	= bit.band(tripFlags, 8) ~= 0
local triggerPlayer  = bit.band(tripFlags, 32) ~= 0
local hasFired = false

ent:bind( "tick", function(self)
	if not physicsBody:initialized() then return end
	if triggerOnce and hasFired then return end

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
			local otherEnt = other:getObject()
			local otherUid = otherEnt:uid()
			currentCollisions[otherUid] = true

			if triggerPlayer and otherEnt:name() ~= "Player" then return end

			if not touching[otherUid] then
				touching[otherUid] = true
				if triggerOnEnter then
					ent:queueHook("link:Broadcast.%UID%", { message = "TurnOn" }, 0)
					if triggerOnce then hasFired = true end
				end
			end
		end
	end

	for uid, _ in pairs(touching) do
		if not currentCollisions[uid] then
			touching[uid] = nil
			if triggerOnExit and not (triggerOnce and hasFired) then
				ent:queueHook("link:Broadcast.%UID%", { message = "TurnOn" }, 0)
				if triggerOnce then hasFired = true end
			elseif triggerOnEnter then
				ent:queueHook("link:Broadcast.%UID%", { message = "TurnOff" }, 0)
			end
		end
	end
end )