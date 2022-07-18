local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local transform = ent:getComponent("Transform")
local physicsState = ent:getComponent("PhysicsState")
local camera = ent:getComponent("Camera")
local cameraTransform = camera:getTransform()

-- setup all timers
local timers = {
	use = Timer.new(),
	holp = Timer.new(),
	flashlight = Timer.new(),
	physcannon = Timer.new()
}
if not timers.use:running() then timers.use:start(); end
if not timers.holp:running() then timers.holp:start(); end
if not timers.flashlight:running() then timers.flashlight:start(); end
if not timers.physcannon:running() then timers.physcannon:start(); end

-- setup held object locals
local heldObject = {
	uid = 0,
	distance = 0,
	smoothSpeed = 4,
	scrollSpeed = 16,
	momentum = Vector3f(0,0,0),
	rotate = false,
}
-- setup light locals
local light = {
	entity = nil
}
for k, v in pairs(ent:getChildren()) do
	if v:name() == "Light" then
		light.entity = v
	end
end

if light.entity == nil then
	light.entity = ent:loadChild("./playerLight.json",true)
end
light.metadata = light.entity:getComponent("Metadata")
light.transform = light.entity:getComponent("Transform")
light.power = light.metadata["light"]["power"]
light.origin = Vector3f(light.transform.position)
light.entity:setComponent("Metadata", { light = { power = 0 } })

-- sound emitter
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

local useDistance = 6
local pullDistance = useDistance * 4

-- on tick
ent:bind( "tick", function(self)
	-- eye transform
	local flattenedTransform = cameraTransform:flatten()
	flattenedTransform.forward = ( transform.forward + Vector3f( 0, cameraTransform.forward.y, 0 ) ):normalize();

	-- toggle flashlight
	if light.enabled then
		local center = flattenedTransform.position
		local direction = flattenedTransform.forward * 4

		local offset = 0.05
		local _, depth = physicsState:rayCast( center, direction )
		if depth >= 0.5 then
			depth = 0.5
		elseif depth < 0 then
			depth = 0.5
		end
		light.transform.position = center + direction * (depth - offset)
	end

	if timers.flashlight:elapsed() > 0.5 and inputs.key("F") then
		timers.flashlight:reset()

		local metadata = { light = { power = light.power } }
		if light.entity:getComponent("Metadata")["light"]["power"] ~= light.power then
			metadata["light"]["power"] = light.power
			light.enabled = true
		else
			metadata["light"]["power"] = 0
			light.enabled = false
		end
		light.entity:setComponent("Metadata", metadata)

		playSound("flashlight")
	end

	-- fire use ray
	if timers.use:elapsed() > 0.5 and inputs.key("E") then
		timers.use:reset()

		local center = flattenedTransform.position
		local direction = flattenedTransform.forward * useDistance

		local prop, depth = physicsState:rayCast( center, direction )
		local payload = {
			user = ent:uid(),
			uid = prop and prop:uid() or 0,
			depth = depth,
		}
		if prop then
			prop:callHook("entity:Use.%UID%", payload)
		end
		ent:callHook("entity:Use.%UID%", payload)
	end

	-- update HOLP
	if heldObject.uid == 0 then
		local mouse2 = inputs.key("Mouse2");
		if mouse2 then
		--[[
			local center = transform.position + cameraTransform.position
			local direction = transform.forward + Vector3f( 0, cameraTransform.forward.y, 0 )
			direction = direction:normalize() * 4
		]]
			local center = flattenedTransform.position
			local direction = flattenedTransform.forward * pullDistance
			local prop, depth = physicsState:rayCast( center, direction )
			if depth >= 0 and prop and not string.matched( prop:name(), "/^worldspawn/" ) then
				local heldObjectTransform = prop:getComponent("Transform")
				local heldObjectPhysicsState = prop:getComponent("PhysicsState")

				local strength = 500
				local distanceSquared = (heldObjectTransform.position - flattenedTransform.position):magnitude()

				heldObjectPhysicsState:applyImpulse( flattenedTransform.forward * -heldObjectPhysicsState:getMass() * strength / distanceSquared )
				if timers.physcannon:elapsed() > 1.0 then
					timers.physcannon:reset()

					playSound("phys_tooHeavy")
				end
			end
		end
	else
		local mouse1 = inputs.key("Mouse1");
		local mouse3 = inputs.key("Mouse3");
		local wheel = inputs.analog("MouseWheel")

		if wheel ~= 0 then
			heldObject.distance = heldObject.distance + (wheel / 120 * heldObject.scrollSpeed) * time.delta()
		end
		if mouse3 then
			heldObject.rotate = not heldObject.rotate
		end

		local prop = entities.get( heldObject.uid )
		local heldObjectTransform = prop:getComponent("Transform")
		local heldObjectPhysicsState = prop:getComponent("PhysicsState")

		if mouse1 and timers.physcannon:elapsed() > 0.5 then
			timers.physcannon:reset()

			heldObject.uid = 0
			heldObjectPhysicsState:enableGravity(true)
			heldObjectPhysicsState:applyImpulse( flattenedTransform.forward * heldObjectPhysicsState:getMass() * 1000 )

			playSound("phys_launch"..math.random(1,4))
		else
			if heldObject.rotate then
				heldObjectTransform.orientation = Quaternion.lookAt( (heldObjectTransform.position - flattenedTransform.position):normalize(), transform.up )
			end
			
			local forward = flattenedTransform.forward * heldObject.distance --flattenedTransform.orientation:rotate( Vector3f(0,0,1) )
			if heldObject.smoothSpeed ~= 0 then
				local target = flattenedTransform.position + forward
				local offset = target - heldObjectTransform.position
				local delta = offset * time.delta() * heldObject.smoothSpeed

				local distance = delta:norm()
				if distance > 0.001 then
					if timers.holp:elapsed() > 0.125 then
						timers.holp:reset()
						heldObjectPhysicsState:setVelocity( delta * 20 )
					end
				else
					heldObjectPhysicsState:setVelocity( Vector3f(0,0,0) )
				end
			else
				heldObjectTransform.position = flattenedTransform.position + forward
			end
		end
	end
end )
-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user ~= ent:uid() then return end

	local validUse = false
	
	if heldObject.uid == 0 and payload.depth > 0 then
		local prop = entities.get( payload.uid )
		local propMetadata = prop:getComponent("Metadata")
		if propMetadata["holdable"] then
			validUse = true
			local offset = transform.position - prop:getComponent("Transform").position

			heldObject.uid = payload.uid
			heldObject.distance = offset:norm()
		
			prop:getComponent("PhysicsState"):enableGravity(false)
		else
			validUse = not string.matched( prop:name(), "/^worldspawn/" )
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
	end

	if validUse then
		playSound("select")
	else
		playSound("deny")
	end
end )