local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end

local polarity = 1
local state = 0
local targetAlpha = 1
local alpha = 0
local target = Vector3f(0,0,0)
local transform = ent:getComponent("Transform")
local metadata = ent:getComponent("Metadata")
local speed = metadata["speed"] or 1.0
local normal = Vector3f(0,0,-1)
if metadata["normal"] ~= nil then
	local sign = -1
	if metadata["angle"] < 0 then sign = 1 end
	normal = Vector3f( metadata["normal"][1] * sign, metadata["normal"][2] * sign, metadata["normal"][3] * sign ):normalize()
end
local starting = Quaternion(transform.orientation)
local ending = transform.orientation:multiply(Quaternion.axisAngle( Vector3f(0,1,0), metadata["angle"] ))
local soundEmitter = ent:loadChild("./sound.json",true)
local playSound = function( key, loop )
	if not loop then loop = false end
	local url = "/door/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( key )
	local url = "/door/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end
local playSoundscape = function( key )
	local url = "/soundscape/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = false,
		volume = "sfx",
		loop = true,
		streamed = true
	}, 0)
end
local stopSoundscape = function( key )
	local url = "/soundscape/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end
soundEmitter:getComponent("Transform"):setReference( transform )
-- on tick
ent:bind( "tick", function(self)
--	transform.orientation = starting:slerp( ending, math.cos(time.current() * speed) * 0.5 + 0.5 )
	if state == 1 then
		alpha = alpha + time.delta() * speed

		if alpha > targetAlpha then
			state = 2
			alpha = targetAlpha
			playSound("default_stop")
		end

	end
	if state == 3 then
		alpha = alpha - time.delta() * speed

		if alpha < 0 then
			state = 0
			alpha = 0
			playSound("default_stop")
		end
	end

	if state > 0 then
		transform.orientation = starting:slerp( ending, alpha * polarity )
	end
end )
-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	if payload.user == ent:uid() then return end

--	if timer:elapsed() <= 0.125 then return end
--	timer:reset()

	print("Processing use: " .. payload["uid"] .. " | " .. payload["depth"] )

	if state == 0 or state == 3 then
		state = 1
		playSound("default_move")
		if payload.uid ~= nil then
			local user = entities.get( payload.user )
			local userTransform = user:getComponent("Transform")
			local delta = transform.position - userTransform.position
			local side = normal:dot(delta)
			if side > 0 then
				polarity = 1
			elseif side < 0 then
				polarity = -1
			end
		end
	elseif state == 2 --[[or state == 1]] then
		state = 3
		playSound("default_move")
	end
end )