local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end
-- release control
--[[
hooks.call("window:Mouse.CursorVisibility", { state = true })
hooks.call("window:Mouse.Lock", {});
]]
-- on tick
ent:bind( "tick", function(self)
	-- update static effect
	local metadatas = {
		ent = ent:getComponent("Metadata"),
		scene = scene:getComponent("Metadata")
	}
	local transforms = {
		ent = ent:getComponent("Transform"),
		controller = controller:getComponent("Transform")
	}
	local distanceSquared = transforms.controller.position:distance( transforms.ent.position )
	distanceSquared = distanceSquared * distanceSquared
	if type(metadatas.ent["static"]["scale"]) == "number" then
		distanceSquared = distanceSquared * metadatas.ent["static"]["scale"]
	end
	local lo = type(metadatas.ent["static"]["range"][1]) == "number" and metadatas.ent["static"]["range"][1] or 0.1
	local hi = type(metadatas.ent["static"]["range"][2]) == "number" and metadatas.ent["static"]["range"][2] or 0.5
	local staticBlend = distanceSquared > 1 and 1.0 / distanceSquared or 1.0
	staticBlend = math.clamp( staticBlend, lo, hi )

	local flicker = type(metadatas.ent["static"]["flicker"]) == "number" and metadatas.ent["static"]["flicker"] or 0.001
	local pieces = type(metadatas.ent["static"]["pieces"]) == "number" and metadatas.ent["static"]["pieces"] or 1000

	local payload = {
		mode = 2,
		parameters = {
			[1] = flicker,
			[2] = pieces,
			[3] = staticBlend,
			[4] = "time"
		}
	}
	scene:callHook("shader:Update.%UID%", payload)
end )

--[[
local hud = ent:loadChild("/hud.json", true)
hud:bind( "tick", function(self)
	-- update distance HUD element
	if timer:elapsed() <= 0.0125 then return end
	timer:reset()
		
	local metadata = self:getComponent("Metadata")
	local transforms = {
		controller = controller:getComponent("Transform"),
		source = ent:getComponent("Transform")
	}
	local distance = transforms.controller.position:distance( transforms.source.position )
	--distance = string.si( distance, "m" )
	local maximum = 40
	local value = math.floor(distance/maximum * 100)
	if value >= 100 then value = 100 end
	distance = "Sanity: " .. value .. "%"
	--distance = math.floor((15-distance)/15 * 100) .. "%"
	if metadata["text settings"]["string"] ~= distance then
		self:callHook( "gui:UpdateString.%UID%", {
			string = distance
		} )
	end
end )
]]
--[[
local marker = ent:loadChild("./marker.json", true)
marker:bind( "tick", function(self)
	local transform = marker:getComponent("Transform")
	local parentTransform = ent:getComponent("Transform")
	local controllerTransform = controller:getComponent("Transform")

	local controllerCamera = controller:getComponent("Camera")
	local controllerCameraTransform = controllerCamera:getTransform()
	
	transform.position = parentTransform.position + Vector3f(0,3,0)
	transform.orientation = Quaternion.lookAt( transform.position - controllerTransform.position, Vector3f(0,1,0) )
	-- transform.orientation = transform.orientation:normalize()
	-- transform.model = controllerCamera:getProjection() * controllerCamera:getView() * Matrix4f.translate( transform.position ) * transform.orientation:matrix()
end )
]]