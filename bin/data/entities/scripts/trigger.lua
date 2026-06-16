local ent = ent
local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}
local physicsBody = ent:getComponent("PhysicsBody")

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

local classname = metadataValve["classname"] or "trigger_multiple"
local wait = tonumber(metadataValve["wait"]) or 0.2
local isActive = true

if classname == "trigger_once" then
	wait = -1
end

local touching = {}
local nextTriggerTime = 0

ent:bind( "tick", function(self)
	if not isActive or not physicsBody:initialized() then return end

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
				ent:queueHook("io:FireOutput.%UID%", { output = "OnStartTouch" }, 0)

				if wait == -1 or timer:elapsed() >= nextTriggerTime then
					ent:queueHook("io:FireOutput.%UID%", { output = "OnTrigger" }, 0)

					if wait == -1 then
						isActive = false
						--entities.destroy(ent)
						return
					else
						nextTriggerTime = timer:elapsed() + wait
					end
				end
			end
		end
	end

	for uid, _ in pairs(touching) do
		if not currentCollisions[uid] then
			touching[uid] = nil
			ent:queueHook("io:FireOutput.%UID%", { output = "OnEndTouch" }, 0)
		end
	end
end )