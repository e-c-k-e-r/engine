local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}
local physicsBody = ent:getComponent("PhysicsBody")

local controller = entities.controller()
local controllerBody = controller:getComponent("PhysicsBody")

local climbSpeed = 2.0

ent:bind( "tick", function(self)
	if not physicsBody:initialized() then return end
	if not controllerBody:initialized() then return end

	local box = physicsBody:bounds()
	local pos = controllerBody:getTransform().position

	local inside =
		pos.x >= box.min.x - 0.5 and pos.x <= box.max.x + 0.5 and
		pos.z >= box.min.z - 0.5 and pos.z <= box.max.z + 0.5 and
		pos.y >= box.min.y - 0.5 and pos.y <= box.max.y + 1.0

	if not inside then return end

	local velocity = controllerBody:getVelocity()

	if window.keyPressed("Space") or window.keyPressed("W") then
		velocity.y = climbSpeed
	elseif window.keyPressed("S") then
		velocity.y = -climbSpeed
	end

	controllerBody:setVelocity(velocity)
end )
