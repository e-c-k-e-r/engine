local ent = ent
local scene = entities.currentScene()
local physicsBody = ent:getComponent("PhysicsBody")

local touching = {}

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
			local uid = other:getObject():uid()
			currentCollisions[uid] = true

			if not touching[uid] then
				touching[uid] = true
				ent:queueHook("dark:Broadcast.%UID%", { message = "TurnOn" }, 0)
			end
		end
	end

	for uid, _ in pairs(touching) do
		if not currentCollisions[uid] then
			touching[uid] = nil
			ent:queueHook("dark:Broadcast.%UID%", { message = "TurnOff" }, 0)
		end
	end
end )