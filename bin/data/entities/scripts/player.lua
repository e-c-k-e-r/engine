local timer = Timer.new()
if not timer:running() then timer:start(); end

local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local transform = ent:getComponent("Transform")
local camera = ent:getComponent("Camera")
local cameraTransform = camera:getTransform()

local playSound = function( key, loop )
	if not loop then loop = false end
	local url = "/ui/" .. key .. ".ogg"
	ent:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
	ent:queueHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end

local heldObject = {
	uid = 0,
	distance = 0,
	smoothSpeed = 4,
	scrollSpeed = 16,
	momentum = Vector3f(0,0,0)
}

-- on tick
ent:bind( "tick", function(self)
	if heldObject.uid ~= 0 then
		local wheel = inputs.analog("MouseWheel")
		local mouse3 = inputs.key("Mouse3");
		if wheel ~= 0 then
			heldObject.distance = heldObject.distance + (wheel / 120 * heldObject.scrollSpeed) * time.delta()
		end
		if mouse3 then
			heldObject.rotate = not heldObject.rotate
		end

		local prop = entities.get( heldObject.uid )
		local heldObjectTransform = prop:getComponent("Transform")
		local heldObjectPhysicsState = prop:getComponent("PhysicsState")
		if heldObject.rotate then
			heldObjectTransform.orientation = transform.orientation
		end
		
		local transform = cameraTransform:flatten()
		local forward = transform.orientation:rotate( Vector3f(0,0,1) ) * heldObject.distance

		if heldObject.smoothSpeed ~= 0 then
			local target = transform.position + forward
			local offset = target - heldObjectTransform.position
			local delta = offset * time.delta() * heldObject.smoothSpeed
			local distance = delta:norm()

			if distance > 0.001 then
				heldObjectTransform.position = heldObjectTransform.position + delta
				heldObject.momentum = offset * 1000
			else
				heldObjectTransform.position = target
			end
		else
			heldObjectTransform.position = transform.position + forward
		end
	end
end )
-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user ~= ent:uid() then return end

	local validUse = false

	if heldObject.uid == 0 then
		local prop = entities.get( payload.uid )
		local propMetadata = prop:getComponent("Metadata")
		if propMetadata["holdable"] then
			validUse = true
			local offset = transform.position - prop:getComponent("Transform").position

			heldObject.uid = payload.uid
			heldObject.distance = offset:norm()
		
			prop:getComponent("PhysicsState"):enableGravity(false)
		end
	elseif heldObject.uid ~= 0 then
		validUse = true
		local prop = entities.get( heldObject.uid )
		local heldObjectPhysicsState = prop:getComponent("PhysicsState")
		heldObjectPhysicsState:enableGravity(true)
		heldObjectPhysicsState:applyImpulse( heldObject.momentum )
		
		heldObject.uid = 0
		heldObject.distance = 0
		heldObject.momentum = Vector3f(0,0,0)
	elseif payload.uid ~= 0 then
		local hit = entities.get( heldObject.uid )
		validUse = not string.match( hit:name(), "/^worldspawn_/" )
	end

	if validUse then
		playSound("select")
	else
		playSound("deny")
	end
end )