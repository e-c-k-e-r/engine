local scene = entities.currentScene()
local controller = entities.controller()
local metadata = ent:getComponent("Metadata")
local masterdata = scene:getComponent("Metadata")

local visorLayers = 5
local children = {}
for i=1, visorLayers do
	children[i] = ent:loadChild("./overlay.json",true)
end

local soundEmitter = ent:loadChild("./sound.json",true)

local timer = Timer.new()
if not timer:running() then timer:start() end

Static = {
	values = {},
	get = function( obj )
		if obj == nil then
			obj = scene
		end
		if Static.values[""..obj:uid()] == nil then
			Static.values[""..obj:uid()] = {}
		end
		return Static.values[""..obj:uid()]
	end
}

local lerper = {
	to = Quaternion(0,0,0,1),
	from = Quaternion(0,0,0,1),
	a = 0
}

local rotate = function( delta )	
	delta.x = delta.x * 0.1
	delta.y = delta.y * 0.1

	local transform = ent:getComponent("Transform")
	local rotation = {
		x = Quaternion.axisAngle( Vector3f(0, 1, 0), delta.x ),
		y = Quaternion.axisAngle( Vector3f(1, 0, 0), delta.y )
	}

	lerper.a = 0
	lerper.from = Quaternion.multiply( transform.orientation, rotation.x:multiply(rotation.y) )
	transform.orientation = lerper.from
	for k, obj in pairs(children) do
		obj:getComponent("Transform").orientation = transform.orientation
	end
end

local windowSize = masterdata["system"]["config"]["window"]["size"];
local entTransform = ent:getComponent("Transform")
entTransform.scale.x = entTransform.scale.x * windowSize.x / windowSize.y;

for k, obj in pairs(children) do
	local transform = obj:getComponent("Transform")
	transform.scale = entTransform.scale;
	transform.position.z = -0.5 + ((k-1) * 0.005)
end

ent:addHook( "window:Resized", function( payload )
	if entTransform.scale.y == entTransform.scale.z then
		entTransform.scale.x = entTransform.scale.y * payload["window"]["size"]["x"] / payload["window"]["size"]["y"];
	end

	for k, obj in pairs(children) do
		local transform = obj:getComponent("Transform")
		transform.scale = entTransform.scale;
	end
end )

ent:addHook( "controller:Camera.Rotated", function( payload )
	rotate( {
		x = -payload.angle.yaw,
		y = -payload.angle.pitch
	})
--[[
	local transform = ent:getComponent("Transform")
	lerper.a = 0
	local counterOrientation = Quaternion( payload.delta[1], payload.delta[2], payload.delta[3], payload.delta[4] ):inverse()
	lerper.from = Quaternion.multiply( transform.orientation, counterOrientation )
	transform.orientation = lerper.from
	for k, obj in pairs(children) do
		obj:getComponent("Transform").orientation = transform.orientation
	end
]]
end )
--[[
ent:addHook( "window:Mouse.Moved", function( payload )
	if payload.invoker ~= "client" then return end

	local delta = payload.mouse.delta
	local size = payload.mouse.size

	if delta == nil or size == nil then return end
	if delta.x == 0 or delta.y == 0 then return end

	delta.x = -delta.x / size.x 
	delta.y = -delta.y / size.y 

	rotate( delta )
end )
]]

ent:bind( "tick", function(self)
	for k, obj in pairs(children) do
		local metadata = obj:getComponent("Metadata")
		local glow = math.sin(time.current()) * 0.5 + 0.5 -- constrained to [0,1]
		glow = glow * 0.2 + 0.65 -- constrained to [0.65, 0.85]
		metadata["alpha"] = glow
		obj:setComponent("Metadata", metadata)
	end

	local controllerTransform = controller:getComponent("Transform")
	local controllerCamera = controller:getComponent("Camera")
	local controllerCameraTransform = controllerCamera:getTransform()
	local transform = ent:getComponent("Transform")
	
	local speed = 2.5
	if lerper.a == 1 then return end
	lerper.a = lerper.a + time.delta() * speed
	if lerper.a > 1 then lerper.a = 1 end

	transform.orientation = lerper.from:slerp( lerper.to, lerper.a )
	local orientation = transform.orientation
	for k, obj in pairs(children) do
		local transform = obj:getComponent("Transform")
		transform.orientation = orientation
		transform.model = controllerCamera:getProjection() * Matrix4f.translate( transform.position ) * transform.orientation:matrix() * Matrix4f.scale( transform.scale ) --Matrix4f.scale( Vector3f( 1.7776 * 2, 2, 2 ) )
	end
end )

--[[
	controller:callHook( "object:UpdateMetadata.%UID%", {
		path = "overlay.position",
		value = {
			[1] = position.x,
			[2] = position.y,
			[3] = position.z
		}
	} )
	controller:callHook( "object:UpdateMetadata.%UID%", {
		path = "overlay.orientation",
		value = {
			[1] = orientation.x,
			[2] = orientation.y,
			[3] = orientation.z,
			[4] = orientation.w
		}
	} )
]]