local ent = ent
local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end

local target = Vector3f(0,0,0)
local transform = ent:getComponent("Transform")
local metadata = ent:getComponent("Metadata")
local physics = ent:getComponent("Physics")
-- local velocty = physics:linearVelocity()
local speed = metadata["speed"] or 1.0
local starting = transform.position + Vector3f(0,0,0)
local ending = transform.position + Vector3f( metadata["delta"][1], metadata["delta"][2], metadata["delta"][3] )
local wait = 0
local direction = 1.0 / math.abs(starting:distance(ending))
local alpha = 0
local startingSound = true

local soundEmitter = ent:loadChild("/sound.json",true)
local playSound = function( key, loop )
	if not loop then loop = false end
	local url = "./audio/sfx/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( key )
	local url = "./audio/sfx/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end
local playSoundscape = function( key )
	local url = "./audio/soundscape/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = false,
		volume = "sfx",
		loop = true,
		streamed = true
	}, 0)
end
local stopSoundscape = function( key )
	local url = "./audio/soundscape/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end
soundEmitter:getComponent("Transform"):setReference( transform )
-- on tick
ent:bind( "tick", function(self)

	if wait > 0 then
		wait = wait - time.delta()
	else
		if startingSound then
			playSound("lift_start", true)
			startingSound = false
		end
		alpha = alpha + time.delta() * speed * direction
		if alpha <= 0 or alpha >= 1 then
			alpha = math.clamp( alpha, 0.0, 1.0 )
			direction = -direction
			wait = 6
			stopSound("lift_start")
			playSound("lift_stop")
			startingSound = true
		--	physics:setLinearVelocity( Vector3f(0,0,0) )
		else
		--	physics:setLinearVelocity( Vector3f(0,direction / math.abs(direction),0) )
		end
	end
	transform.position = Vector3f.lerp( starting, ending, alpha )
end )