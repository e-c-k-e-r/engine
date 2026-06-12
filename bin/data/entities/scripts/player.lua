local ent = ent
local scene = entities.currentScene()
local graph = scene:getComponent("Graph")
local metadataJson = ent:getComponent("Metadata")
local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local camera = ent:getComponent("Camera")
local cameraTransform = camera:getTransform()
local metadata = ent:getComponent("PlayerBehavior::Metadata")
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
local playSound = function( path, loop )
	if not loop then loop = false end
	local uri = string.resolveURI(path)
	ent:callHook("sound:Emit.%UID%", {
		filename = uri,
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( path )
	ent:callHook("sound:Stop.%UID%", {
		filename = string.resolveURI(path, metadataJson["system"]["root"])
	}, 0)
end
local playUiSound = function( key, loop )
	return playSound("/ui/" .. key .. ".ogg", loop)
end
local stopUiSound = function( key )
	return stopSound("/ui/" .. key .. ".ogg", loop)
end

local useDistance = 6
local pullDistance = useDistance * 4

local function tickFlashlight( transform, axes, inputs )
	-- update light position
	if light.enabled then
		local center = transform.position
		local direction = axes.forward * 8
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
		playUiSound("flashlight")
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

	playUiSound(validUse and "select" or "deny")
end

local function tickUse( transform, axes, inputs )
	-- trigger use
	if timers.use:elapsed() > 0.5 and inputs["E"] then
		timers.use:reset()
		local center = transform.position
		local direction = axes.forward * useDistance
		local hit, depth = physicsBody:rayCast(center, direction)

		local payload = {
			user = ent:uid(),
			uid = hit and hit:uid() or 0,
			depth = depth,
		}
		if hit then hit:lazyCallHook("entity:Use.%UID%", payload) end
		ent:lazyCallHook("entity:Use.%UID%", payload)
	end
end

local function tickGravGun( transform, axes, inputs )
	-- not holding anything
	if heldObject.uid == 0 then
		-- try and launch object in sights
		if inputs["mouse2"] then
			local center = transform.position
			local direction = axes.forward * pullDistance
			local prop, depth = physicsBody:rayCast( center, direction )
			if depth >= 0 and prop and not string.matched( prop:name(), "/^worldspawn/" ) then
				local heldObjectTransform = prop:getComponent("Transform")
				local heldObjectPhysicsBody = prop:getComponent("PhysicsBody")

				local strength = 500
				local distanceSquared = (heldObjectTransform.position - transform.position):magnitude()

				heldObjectPhysicsBody:applyImpulse( axes.forward * -heldObjectPhysicsBody:getMass() * strength / distanceSquared )
				if timers.physcannon:elapsed() > 1.0 then
					timers.physcannon:reset()

					playUiSound("phys_tooHeavy")
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
			heldObjectPhysicsBody:applyImpulse( axes.forward * heldObjectPhysicsBody:getMass() * 50 )

			playUiSound("phys_launch"..math.random(1,4))
		else
			-- update rotation
			if heldObject.rotate then
				--heldObjectTransform.orientation = Quaternion.lookAt( (heldObjectTransform.position - transform.position):normalize(), axes.up )
				heldObjectTransform.orientation = cameraTransform:flatten().orientation
			end
			
			-- move held object
			local forward = axes.forward * heldObject.distance
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

local footstepTimer = 0.0
local surfaceTypes = {
	"chainlink",
	"concrete",
	"dirt",
	"duct",
	"grass",
	"gravel",
	"hardboot_generic",
	"ladder",
	"metal",
	"metalgrate",
	"mud",
	"sand",
	"slosh",
	"snow",
	"softshoe_generic",
	"tile",
	"wade",
	"wood",
	"woodpanel"
}

local playFootstepSound = function( surface, isCrouching )
	local variant = math.random(1, 4)
	local path = "valve://sound/player/footsteps/" .. surface .. tostring(variant) .. ".wav"
	local pitch = 0.95 + (math.random() * 0.10)
	local vol = 1.0
	
	if isCrouching then vol = 0.5 end

	ent:callHook("sound:Emit.%UID%", {
		filename = string.resolveURI(path, metadataJson["system"]["root"]),
		spatial = true,
		streamed = false,
		volume = vol,
		pitch = pitch,
		loop = false
	}, 0)
end

local tickFootsteps = function()
	local isWalking = metadata.states.walking
	local isRunning = metadata.states.running
	local isCrouching = metadata.states.crouching
	local isFloored = metadata.states.floored
	local isNoclipped = metadata.states.noclipped

	if not isFloored or isNoclipped then return end

	if not isWalking then
		footstepTimer = 0.0
		return
	end

	footstepTimer = footstepTimer - time.delta()
	if footstepTimer > 0.0 then return end

	local surface = "concrete"
	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs( collisionEvents ) do
		if event.normal.y <= -0.7 then
			local tri = event.featureA or event.featureB
			local other = event.a == ent and event.a or event.b
			local collider = other:getCollider()

			if tri ~= nil and collider.type == ShapeType.MESH then
				local mesh = collider:asMesh()
				local drawCommand = mesh:fetchDrawCommand( tri )
				local instance = graph:getInstance( drawCommand.instanceID )
				local materialName = string.lower( graph:getMaterialName( instance.materialID ) )
				for _, key in ipairs(surfaceTypes) do
					if string.find(materialName, key) then
						surface = key
						break
					end
				end

				break
			end
		end
	end

	playFootstepSound( surface, isCrouching )


	if isRunning then
		footstepTimer = 0.3
	elseif isCrouching then
		footstepTimer = 0.6
	else
		footstepTimer = 0.45
	end
end

--[[
local tickCollisionEvents = function()
	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs(collisionEvents) do
	--	print( event.state, event.a, event.b, event.point, event.normal, event.impulse, event.featureA )
		local tri = event.featureA or event.featureB
		local other = event.a == ent and event.a or event.b
		local collider = other:getCollider()
		-- technically will always return a triangle ID if there's a mesh collider
		if tri ~= nil and collider.type == ShapeType.MESH then
			local mesh = collider:asMesh()
			local drawCommand = mesh:fetchDrawCommand( tri )
			local instance = graph:getInstance( drawCommand.instanceID )
			local material = graph:getMaterial( instance.materialID )
			local materialName = graph:getMaterialName( instance.materialID )
			if not materialName:find("tools/") and event.normal.y < -0.7 then
				local soundKey = getSurfaceSound(materialName)
				playSound("valve://sound/" .. soundKey .. ".wav", false)
			end
		end
	end
end
]]

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
	local axes = flattenedTransform:axes()

	-- update flashlight
	tickFlashlight( flattenedTransform, axes, inputs )
	
	-- update use
	tickUse( flattenedTransform, axes, inputs )

	-- update HOLP
	tickGravGun( flattenedTransform, axes, inputs )

	-- play footsteps
	tickFootsteps()
end )

-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user ~= ent:uid() then return end
	onUse( payload )
end )