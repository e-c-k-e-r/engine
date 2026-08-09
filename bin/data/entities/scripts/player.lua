local ent = ent
local scene = entities.currentScene()
local graph = scene:getComponent("Graph")
local physicsBody = ent:getComponent("PhysicsBody")
local metadataJson = ent:getComponent("Metadata")
local camera = ent:getComponent("Camera")
local cameraMetadata = ent:getComponent("PlayerCameraBehavior::Metadata")
local moveStates = ent:getComponent("PlayerMovementBehavior::Metadata")
local fixedCamera = cameraMetadata and cameraMetadata.fixed or false

-- setup all timers
local timers = {
	holp = Timer.new(),
	flashlight = Timer.new(),
	physcannon = Timer.new()
}
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

-- setup light
local light = { entity = nil }
for k, v in pairs(ent:getChildren()) do
	if type(v) == "table" and v:name() == "Light" then light.entity = v end
end
if light.entity == nil then
	light.entity = ent:loadChild("ent://playerLight.json",true)
end

light.metadata = light.entity:getComponent("LightBehavior::Metadata")
light.transform = light.entity:getComponent("Transform")
light.power = light.metadata.power
light.metadata.power = 0

-- UI sound helpers
local playUiSound = function( key, loop )
	ent:callHook("sound:Emit.%UID%", {
		filename = string.resolveURI("/ui/" .. key .. ".ogg"),
		spatial = true, streamed = true, volume = "sfx", loop = loop or false
	}, 0)
end

local pullDistance = 24

local function tickFlashlight( transform, axes, inputState )
	if light.enabled then
		local center = transform.position
		local direction = axes.forward * 8
		local _, depth = physicsBody:rayCast(center, direction)
		depth = math.clamp(depth, 0, 0.5)
		light.transform.position = center + direction * (depth - 0.25)
	end

	-- Flashlight uses 'F' key (we can keep this manual since it's game-specific, or add it to C++ input state)
	if timers.flashlight:elapsed() > 0.5 and inputs.key("F") then
		timers.flashlight:reset()
		light.enabled = (light.metadata.power ~= light.power)
		light.metadata.power = light.enabled and light.power or 0
		playUiSound("flashlight")
	end
end

local function tickGravGun( transform, axes, inputState )
	local mouse1 = inputs.key("Mouse1") or inputs.key("L_TRIGGER")
	local mouse2 = inputs.key("Mouse2") or inputs.key("R_TRIGGER")
	local mouse3 = inputs.key("Mouse3")
	local wheel = inputs.analog("MouseWheel")

	if heldObject.uid == 0 then
		if mouse2 then
			local prop, depth = physicsBody:rayCast( transform.position, axes.forward * pullDistance )
			if depth >= 0 and prop and not string.matched( prop:name(), "/^worldspawn/" ) then
				local propPhysics = prop:getComponent("PhysicsBody")
				local distanceSquared = (prop:getComponent("Transform").position - transform.position):magnitude()
				propPhysics:applyImpulse( axes.forward * -propPhysics:getMass() * 500 / distanceSquared )

				if timers.physcannon:elapsed() > 1.0 then
					timers.physcannon:reset()
					playUiSound("phys_tooHeavy")
				end
			end
		end
	else
		-- Held object logic (unchanged from your original, just cleaner scope)
		if wheel ~= 0 then heldObject.distance = heldObject.distance + (wheel / 120 * heldObject.scrollSpeed) * time.delta() end
		if mouse3 then heldObject.rotate = not heldObject.rotate end

		local prop = entities.get( heldObject.uid )
		local propTransform = prop:getComponent("Transform")
		local propPhysics = prop:getComponent("PhysicsBody")

		if mouse1 and timers.physcannon:elapsed() > 0.5 then
			timers.physcannon:reset()
			heldObject.uid = 0
			propPhysics:enableGravity(true)
			propPhysics:applyImpulse( axes.forward * propPhysics:getMass() * 50 )
			playUiSound("phys_launch"..math.random(1,4))
		else
			if heldObject.rotate then propTransform.orientation = camera:getTransform():flatten().orientation end

			local target = transform.position + (axes.forward * heldObject.distance)
			local offset = target - propTransform:flatten().position
			propPhysics:applyImpulse((offset * 15.0 + propPhysics:getVelocity() * -2.0) * propPhysics:getMass() * time.delta())
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

local function getFloorSurface()
	local surface = "concrete"
	local collisionEvents = physicsBody:getCollisionEvents()
	for i, event in ipairs( collisionEvents ) do
		if event.normal.y <= -0.7 then
			local packedID = event.featureA or event.featureB
			local other = event.a == ent and event.a or event.b
			local collider = other:getCollider()

			if packedID ~= nil and collider.type == ShapeType.MESH then
				local mesh = collider:asMesh()
				local viewID, triID = mesh:unpackID( packedID )
				local drawCommand = mesh:fetchDrawCommand( viewID )
				local instance = graph:getInstance( drawCommand.instanceID )
				local materialName = string.lower( graph:getMaterialName( instance.materialID ) )

				for _, key in ipairs(surfaceTypes) do
					if string.find(materialName, key) then
						return key
					end
				end
			end
			break
		end
	end
	return surface
end

local airTime = 0.0

local tickFootsteps = function( inputState )
	local isWalking = moveStates.walking
	local isRunning = moveStates.running
	local isCrouching = moveStates.crouching
	local isFloored = moveStates.floored
	local isNoclipped = moveStates.noclipped

	if not isNoclipped then
		if not isFloored then
			airTime = airTime + time.delta()
		end

		if isFloored and not wasFloored then
			if airTime > 0.15 then
				playFootstepSound( getFloorSurface(), isCrouching )
				footstepTimer = isRunning and 0.3 or 0.45
			end
			airTime = 0.0
		end

		if not isFloored and wasFloored and inputState.jump then
			playFootstepSound( getFloorSurface(), isCrouching )
		end
	end
	wasFloored = isFloored

	if not isWalking or isNoclipped then
		footstepTimer = 0.0
		return
	end

	footstepTimer = footstepTimer - time.delta()

	if not isFloored then return end

	if footstepTimer > 0.0 then return end

	playFootstepSound( getFloorSurface(), isCrouching )

	if isRunning then
		footstepTimer = 0.3
	elseif isCrouching then
		footstepTimer = 0.6
	else
		footstepTimer = 0.45
	end
end

ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user ~= ent:uid() then return end

	print( "Use:", entities.get( payload.uid ) )

	local validUse = false
	if heldObject.uid == 0 and payload.depth > 0 then
		local prop = entities.get( payload.uid )
		if prop:getComponent("Metadata")["holdable"] then
			validUse = true
			heldObject.uid = payload.uid
			heldObject.distance = (ent:getComponent("Transform").position - prop:getComponent("Transform"):flatten().position):norm()
			prop:getComponent("PhysicsBody"):enableGravity(false)
		else
			validUse = not string.matched( prop:name(), "/^worldspawn/" )
		end
	elseif heldObject.uid ~= 0 then
		validUse = true
		local prop = entities.get( heldObject.uid )
		prop:getComponent("PhysicsBody"):enableGravity(true)
		heldObject.uid = 0
	end
	playUiSound(validUse and "select" or "deny")
end )

-- on tick
ent:bind( "tick", function(self)
	local inputs = ent:getComponent("PlayerInputBehavior::Metadata")
	if not inputs or not inputs.control then return end

	local flattenedTransform = fixedCamera and ent:getComponent("Transform"):flatten() or camera:getTransform():flatten()
	local axes = flattenedTransform:axes()

	-- update flashlight
	tickFlashlight( flattenedTransform, axes, inputs )
	
	-- update HOLP
	tickGravGun( flattenedTransform, axes, inputs )

	-- play footsteps
	tickFootsteps( inputs )
end )