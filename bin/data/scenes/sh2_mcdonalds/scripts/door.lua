local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end

local target = Vector3f(0,0,0)
local transform = ent:getComponent("Transform")
local metadata = ent:getComponent("Metadata")
local speed = metadata["speed"] or 1.0 / 3.0
local starting = Quaternion(transform.orientation)
local ending = transform.orientation:multiply(Quaternion.axisAngle( Vector3f(0,1,0), metadata["angle"] ))
-- on tick
ent:bind( "tick", function(self)
	transform.orientation = starting:slerp( ending, math.cos(time.current() * speed) * 0.5 + 0.5 )
end )