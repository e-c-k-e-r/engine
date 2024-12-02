local ent = ent
local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local transform = ent:getComponent("Transform")
local physicsState = ent:getComponent("PhysicsState")
local camera = ent:getComponent("Camera")
local cameraTransform = camera:getTransform()

-- setup all timers
local timers = {
	lookat = Timer.new(),
}
if not timers.lookat:running() then timers.lookat:start(); end

--[[
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
	ent:callHook("sound:Emit.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = true,
		streamed = true,
		volume = "sfx",
		loop = loop
	}, 0)
end
local stopSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
	ent:callHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	}, 0)
end
]]

local collider = ent:getComponent("PhysicsState")
local target_transform = nil
-- on tick
ent:bind( "tick", function(self)
	-- rotate to target
	if target_transform ~= nil then	
		local target = (target_transform.position - transform.position):normalize()
		local dot = transform.forward:dot( target )
		if dot < 1.0 then
			local cross = Vector3f.cross( transform.forward, target ):normalize()
			local axis = transform.up
			local angle = Vector3f.signedAngle( transform.forward, target, axis ) 
			local rot = Quaternion.axisAngle( axis, angle * time.delta() * 4 )

			if collider:hasBody() then
				collider:applyRotation( rot )
			else
				transform:rotate( rot )
			end
		end

		if timers.lookat:elapsed() > 2.0 then
			timers.lookat:reset()
			target_transform = nil
		end
	end
end )
-- on use
ent:addHook( "entity:Use.%UID%", function( payload )
	-- get dialogue text
	local texts = {
		"Lorem ipsum dolor sit amet,\nconsectetur adipiscing elit,\nsed do eiusmod tempor incididunt\nut labore et dolore magna aliqua.",
		"The birch canoe slid on\nthe smooth planks.",
		"Glue the sheet to the dark\nblue background.",
		"It's easy to tell the depth\nof a well.",
		"These days a chicken leg\nis a rare dish.",
		"Rice is often served in\nround bowls.",
		"The juice of lemons makes\nfine punch.",
		"The box was thrown beside\nthe parked truck.",
		"The hogs were fed chopped\ncorn and garbage.",
		"Four hours of steady work\nfaced us.",
		"A large size in stockings\nis hard to sell.",
		"The boy was there when\nthe sun rose.",
		"A rod is used to catch\npink salmon.",
		"The source of the huge river\nis the clear spring.",
		"Kick the ball straight and\nfollow through.",
		"Help the woman get back to\nher feet.",
		"A pot of tea helps to pass\nthe evening.",
		"Smoky fires lack flame\nand heat.",
		"The soft cushion broke the\nman's fall.",
		"The salt breeze came across\nfrom the sea.",
		"The girl at the booth sold\nfifty bonds.",
		"The small pup gnawed a hole\nin the sock.",
		"The fish twisted and turned\non the bent hook.",
		"Press the pants and sew a\nbutton on the vest.",
		"The swan dive was far short\nof perfect.",
		"The beauty of the view\nstunned the young boy.",
		"Two blue fish swam in\nthe tank.",
		"Her purse was full of\nuseless trash.",
		"The colt reared and threw\nthe tall rider.",
		"It snowed, rained, and\nhailed the same morning.",
		"Read verse out loud\nfor pleasure.",
	}

	local text = texts[math.random( #texts )] or "!! Test Message !!"

	local forward = {
		name = "dialogue",
		metadata = {
			dialogue = {
				name = metadata["name"],
				text = text
			}
		}
	}
	scene:callHook("menu:Open", forward)

	-- rotate to target
	local user = scene:globalFindByUid(payload["user"])
	
	target_transform = user:getComponent("Transform")
	timers.lookat:reset()
end )