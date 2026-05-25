local ent = ent
local scene = entities.currentScene()
local metadataJson = ent:getComponent("Metadata")
local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local camera = ent:getComponent("Camera")
local cameraTransform = camera:getTransform()

local fixedCamera = metadataJson["camera"]["settings"]["fixed"]

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
	scrollSpeed = 32,
	momentum = Vector3f(0,0,0),
	rotate = false,
}
-- setup light locals
local light = {
	entity = nil,
}
for k, v in pairs(ent:getChildren()) do
	if type(v) == "number" then
		goto continue
	end
	if v:name() == "Light" then
		light.entity = v
	end
	::continue::
end

if light.entity == nil then
	light.entity = ent:loadChild("./playerLight.json",true)
end
light.metadata = light.entity:getComponent("LightBehavior::Metadata")
light.transform = light.entity:getComponent("Transform")
light.power = light.metadata.power
light.origin = Vector3f(light.transform.position)
light.metadata.power = 0
--light.entity:setComponent("Metadata", { light = { power = 0 } })

-- sound emitter
local playSound = function( key, loop )
	if not loop then loop = false end
	local url = "/ui/" .. key .. ".ogg"
	ent:callHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadataJson["system"]["root"]),
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
	ent:callHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadataJson["system"]["root"])
	}, 0)
end

local useDistance = 6
local pullDistance = useDistance * 4

local function tickFlashlight( transform, inputs )
	-- update light position
	if light.enabled then
		local center = transform.position
		local direction = transform.forward * 8
		local offset = 0.25
		local _, depth = physicsBody:rayCast(center, direction)
		depth = math.clamp(depth, 0, 0.5)
		light.transform.position = center + direction * (depth - offset)
	end

	-- toggle
	if timers.flashlight:elapsed() > 0.5 and inputs["F"] then
		timers.flashlight:reset()
		light.enabled = (light.metadata.power ~= light.power)
		light.metadata.power = light.enabled and light.power or 0
		playSound("flashlight")
	end
end

local function onUse( payload )
	local validUse = false
	-- not currently holding anything, and hit something
	if heldObject.uid == 0 and payload.depth > 0 then
		local prop = entities.get( payload.uid )
		local propMetadata = prop:getComponent("Metadata")
		-- entity is holdable, pick it up
		if propMetadata["holdable"] then
			validUse = true
			local heldObjectTransform = prop:getComponent("Transform")
			local heldObjectFlattened = heldObjectTransform:flatten()
			local offset = transform.position - heldObjectFlattened.position

			heldObject.uid = payload.uid
			heldObject.distance = offset:norm()
			
			local heldObjectPhysicsBody = prop:getComponent("PhysicsBody")
			heldObjectPhysicsBody:enableGravity(false)
		else
			validUse = not string.matched( prop:name(), "/^worldspawn/" )
		end
	-- currently holding something, drop it
	elseif heldObject.uid ~= 0 then
		validUse = true
		local prop = entities.get( heldObject.uid )
		local heldObjectPhysicsBody = prop:getComponent("PhysicsBody")
		heldObjectPhysicsBody:enableGravity(true)
		heldObjectPhysicsBody:applyImpulse( heldObject.momentum )
		
		heldObject.uid = 0
		heldObject.distance = 0
		heldObject.momentum = Vector3f(0,0,0)
	end

	playSound(validUse and "select" or "deny")
end

local function tickUse( transform, inputs )
	-- trigger use
	if timers.use:elapsed() > 0.5 and inputs["E"] then
		timers.use:reset()
		local center = transform.position
		local direction = transform.forward * useDistance
		local prop, depth = physicsBody:rayCast(center, direction)

		local payload = {
			user = ent:uid(),
			uid = prop and prop:uid() or 0,
			depth = depth,
		}
		if prop then prop:lazyCallHook("entity:Use.%UID%", payload) end
		ent:lazyCallHook("entity:Use.%UID%", payload)
	end
end

local function tickGravGun( transform, inputs )
	-- not holding anything
	if heldObject.uid == 0 then
		-- try and launch object in sights
		if inputs["mouse2"] then
			local center = transform.position
			local direction = transform.forward * pullDistance
			local prop, depth = physicsBody:rayCast( center, direction )
			if depth >= 0 and prop and not string.matched( prop:name(), "/^worldspawn/" ) then
				local heldObjectTransform = prop:getComponent("Transform")
				local heldObjectPhysicsBody = prop:getComponent("PhysicsBody")

				local strength = 500
				local distanceSquared = (heldObjectTransform.position - transform.position):magnitude()

				heldObjectPhysicsBody:applyImpulse( transform.forward * -heldObjectPhysicsBody:getMass() * strength / distanceSquared )
				if timers.physcannon:elapsed() > 1.0 then
					timers.physcannon:reset()

					playSound("phys_tooHeavy")
				end
			end
		end
	-- holding something
	else
		-- adjust hold distance
		if inputs["wheel"] ~= 0 then
			heldObject.distance = heldObject.distance + (inputs["wheel"] / 120 * heldObject.scrollSpeed) * time.delta()
		end
		-- update rotation mode
		if inputs["mouse3"] then
			heldObject.rotate = not heldObject.rotate
		end

		local prop = entities.get( heldObject.uid )
		local heldObjectTransform = prop:getComponent("Transform")
		local heldObjectPhysicsBody = prop:getComponent("PhysicsBody")

		-- launch held object
		if inputs["mouse1"] and timers.physcannon:elapsed() > 0.5 then
			timers.physcannon:reset()

			heldObject.uid = 0
			heldObjectPhysicsBody:enableGravity(true)
			heldObjectPhysicsBody:applyImpulse( transform.forward * heldObjectPhysicsBody:getMass() * 50 )

			playSound("phys_launch"..math.random(1,4))
		else
			-- update rotation
			if heldObject.rotate then
				--heldObjectTransform.orientation = Quaternion.lookAt( (heldObjectTransform.position - transform.position):normalize(), transform.up )
				heldObjectTransform.orientation = cameraTransform:flatten().orientation
			end
			
			-- move held object
			local forward = transform.forward * heldObject.distance
			if heldObject.smoothSpeed ~= 0 then
				local heldObjectFlattened = heldObjectTransform:flatten()

				local target = transform.position + forward
				local offset = target - heldObjectFlattened.position

				local stiffness = 15.0
				local damping = 2.0
				local currentVelocity = heldObjectPhysicsBody:getVelocity()
				local mass = heldObjectPhysicsBody:getMass()

				local springForce = offset * stiffness
				local dampingForce = currentVelocity * -damping

				heldObjectPhysicsBody:applyImpulse((springForce + dampingForce) * mass * time.delta())
			else
				heldObjectTransform.position = transform.position + forward
			end
		end
	end
end

-- on tick
ent:bind( "tick", function(self)
	local inControl = scene:globalFindByName("Gui: Menu"):uid() == 0

	local inputs = {
		E = inputs.key("E") or inputs.key("R_Y"),
		F = inputs.key("F"),
		mouse1 = inputs.key("Mouse1") or inputs.key("L_TRIGGER"),
		mouse2 = inputs.key("Mouse2") or inputs.key("R_TRIGGER"),
		mouse3 = inputs.key("Mouse3"),
		wheel = inputs.analog("MouseWheel"),
	}

	if not inControl then
		inputs["E"] = false
		inputs["F"] = false
		inputs["mouse1"] = false
		inputs["mouse2"] = false
		inputs["mouse3"] = false
		inputs["wheel"] = 0
	end

	-- eye transform
	local flattenedTransform = fixedCamera and transform:flatten() or cameraTransform:flatten()

	-- update flashlight
	tickFlashlight( flattenedTransform, inputs )
	
	-- update use
	tickUse( flattenedTransform, inputs )

	-- update HOLP
	tickGravGun( flattenedTransform, inputs )

	-- get collision events
--[[
	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs(collisionEvents) do
		print( event.state, event.a, event.b, event.point, event.normal, event.impulse )
	end
]]
end )

-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user ~= ent:uid() then return end
	onUse( payload )
end )