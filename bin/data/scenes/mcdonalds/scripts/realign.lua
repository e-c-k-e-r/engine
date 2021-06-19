local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end

local transform = ent:getComponent("Transform")
-- on tick
ent:bind( "tick", function(self)
	local offset = Vector3f(0,0,0)
	if window.keyPressed("J") then offset.x = offset.x - time.delta() end
	if window.keyPressed("L") then offset.x = offset.x + time.delta() end
	if window.keyPressed("N") then offset.y = offset.y - time.delta() end
	if window.keyPressed("M") then offset.y = offset.y + time.delta() end
	if window.keyPressed("I") then offset.z = offset.z - time.delta() end
	if window.keyPressed("K") then offset.z = offset.z + time.delta() end
	if offset:magnitude() > 0.0001 then
		transform.position = transform.position + offset
	end
end )