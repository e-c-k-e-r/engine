local scene = entities.currentScene()
local controller = entities.controller()
local controllerTransform = controller:getComponent("Transform")
local timer = Timer.new()
if not timer:running() then
	timer:start();
end
local metadata = ent:getComponent("Metadata")
local soundEmitter = ent:loadChild("./sound.json",true)
local playSound = function( key )
	if not loop then loop = false end
	local url = "./audio/sfx/" .. key .. ".ogg"
	soundEmitter:queueHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true,
		loop = loop,
		volume = "sfx",
		streamed = true
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

local target = Vector3f(0,0,0)
local transform = ent:getComponent("Transform")
local speed = metadata["speed"] or 1.0 / 3.0
local angle = metadata["angle"] or 1.5
transform.orientation = Quaternion.axisAngle( Vector3f(0,1,0), 1.5 )
local starting = transform.orientation:multiply(Quaternion.axisAngle( Vector3f(0,1,0), -angle ))
local ending = transform.orientation:multiply(Quaternion.axisAngle( Vector3f(0,1,0), angle ))
-- on tick
local delta = 0
local watch = 0
local alerted = false
local light = nil
local lightOffset = nil

for k, v in pairs(ent:getChildren()) do
	if v:uid() ~= soundEmitter:uid() then
		light = v
		local transform = v:getComponent("Transform")
		lightOffset = Vector3f(transform.position) --:magnitude()
	end
end

ent:bind( "tick", function(self)
	soundEmitter:getComponent("Transform").position = transform.position

	local angleThreshold = metadata["sensitivity"] or 20

	local controllerPosition = Vector3f(controllerTransform.position.x, 0, controllerTransform.position.z)
	local cameraPosition = Vector3f(transform.position.x, 0, transform.position.z)
	local distance = cameraPosition:distance(controllerPosition)
	local direction = cameraPosition - controllerPosition

	local lightTransform = light and light:getComponent("Transform") or nil
	local lightMetadata = light and light:getComponent("Metadata") or {}
	if lightTransform ~= nil then
		lightTransform.position = transform.orientation:rotate( lightOffset );
		if lightMetadata and lightMetadata["light"] and lightMetadata["light"]["static"] then
			lightTransform.position = lightTransform.position + transform.position
		end
	end

	local angle = math.acos(transform.orientation:rotate( Vector3f(0,0,1):normalize() ):dot( direction:normalize() )) * 180.0 / 3.1415926
	if angle < angleThreshold and distance < 16 then
		if watch == 0 then
			watch = 6
			playSound("camera_see")
			if light then
				light:callHook( "object:UpdateMetadata.%UID%", {
					path = "light.color",
					value = {
						[1] = 1,
						[2] = 1,
						[3] = 0
					}
				} )
			end
		end
		watch = watch + time.delta()
		if watch > 12 and not alerted then
			playSound("camera_alert")
			playSound("xerxes_alert")
			playSoundscape("alarm")
			alerted = true
			timer:reset()
			if light then
				light:callHook( "object:UpdateMetadata.%UID%", {
					path = "light.color",
					value = {
						[1] = 1,
						[2] = 0,
						[3] = 0
					}
				} )
				light:callHook( "object:UpdateMetadata.%UID%", {
					path = "light.fade",
					value = {
						rate = 1,
						power = 0.01,
						timeout= 0.5
					}
				} )
			end
		end
	else
		if watch > 0 and not alerted then
			watch = watch - time.delta()
			if watch < 0 then
				io.print("CAMERA LOST")
				watch = 0
				speed = metadata["speed"] or 1.0 / 3.0
				if light then
					light:callHook( "object:UpdateMetadata.%UID%", {
						path = "light.color",
						value = {
							[1] = 0,
							[2] = 1,
							[3] = 0
						}
					} )
					light:callHook( "object:UpdateMetadata.%UID%", {
						path = "light.fade",
						value = nil
					} )
				end
			end
		end
	end
	if not alerted and watch > 0 and light then
		light:callHook( "object:UpdateMetadata.%UID%", {
			path = "light.fade.rate",
			value = math.floor(watch / 2),
		} )
		light:callHook( "object:UpdateMetadata.%UID%", {
			path = "light.fade.power",
			value = 0.01,
		} )
		light:callHook( "object:UpdateMetadata.%UID%", {
			path = "light.fade.timeout",
			value = 0.5,
		} )
	end
	if alerted and timer:elapsed() >= 60 then
		timer:reset()
		alerted = false
		watch = 0
		io.print("ALERT OVER")
		playSound("camera_lost")
		stopSoundscape("alarm");
		if light then
			light:callHook( "object:UpdateMetadata.%UID%", {
				path = "light.color",
				value = {
					[1] = 0,
					[2] = 1,
					[3] = 0
				}
			} )
			light:callHook( "object:UpdateMetadata.%UID%", {
				path = "light.fade",
				value = nil
			} )
		end
	end
	
	delta = delta + time.delta() * speed
	local nextRotation = starting:slerp( ending, math.cos(delta) * 0.5 + 0.5 )
	-- stop if we are going to look away from player
	local angleNext = math.acos(nextRotation:rotate( Vector3f(0,0,1):normalize() ):dot( direction:normalize() )) * 180.0 / 3.1415926
	if watch > 0 and angleNext > angle then
		delta = delta - time.delta() * speed * 3
		nextRotation = starting:slerp( ending, math.cos(delta) * 0.5 + 0.5 )
	end
	transform.orientation = nextRotation
end )