local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}
local physicsBody = ent:getComponent("PhysicsBody")

local health = tonumber(metadataValve["health"]) or 100.0
local flags = tonumber(metadataValve["spawnflags"]) or 0
local unbreakable = (math.floor(flags / 1) % 2) ~= 0
local damageScale = tonumber(metadataValve["damagescale"]) or 1.0

local alive = true

local function destroy()
	if not alive then return end
	alive = false
	ent:queueHook("io:FireOutput.%UID%", { output = "OnBreak" }, 0)
	entities.destroy(ent)
end

ent:bind( "tick", function(self)
	if not alive or unbreakable then return end
	if not physicsBody:initialized() then return end

	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs(collisionEvents) do
		if event.impulse > 1.0 then
			health = health - (event.impulse * damageScale)
			if health <= 0 then
				destroy()
				break
			end
		end
	end
end )

ent:addHook("io:Input.%UID%", function( payload )
	local input = payload.input

	if input == "Kill" or input == "Break" then
		destroy()
	end
end)
