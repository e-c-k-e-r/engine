local scene = entities.currentScene()
local controller = entities.controller()

local transform = ent:getComponent("Transform")
local controllerTransform = controller:getComponent("Transform")
local offset = Vector3f(0,1,0) -- transform.position
io.print("TRACKING PLAYER")
-- on tick
ent:bind( "tick", function(self)
	transform.position = offset + controllerTransform.position
end )