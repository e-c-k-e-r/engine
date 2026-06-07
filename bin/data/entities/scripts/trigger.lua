local ent = ent
local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}
local physicsBody = ent:getComponent("PhysicsBody")

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

--[[
ent:bind( "tick", function(self)
	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs(collisionEvents) do
		-- do something
		-- print( event.state, event.a, event.b, event.point, event.normal, event.impulse )
	end
end )
]]