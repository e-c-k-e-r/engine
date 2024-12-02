local ent = ent
local scene = entities.currentScene()
local controller = entities.controller()
local metadata = ent:getComponent("Metadata")
local masterdata = scene:getComponent("Metadata")

if metadata["dialogue"] == nil then
	metadata["dialogue"] = {}
end

local children = {
	text = {
		box = ent:loadChild("./text-box.json",true),
		string = ent:loadChild("./text-string.json",true),
	},
	name = {
		box = ent:loadChild("./name-box.json",true),
		string = ent:loadChild("./name-string.json",true),
	}
}

local soundEmitter = ent:loadChild("./sound.json",true)

local timer = Timer.new()
if not timer:running() then timer:start() end

Static = {
	values = {},
	get = function( obj )
		if obj == nil then
			obj = scene
		end
		if Static.values[""..obj:uid()] == nil then
			Static.values[""..obj:uid()] = {}
		end
		return Static.values[""..obj:uid()]
	end
}

local bind = function( text, box, str, color )
	local box_transform = box:getComponent("Transform")
	local text_transform = text:getComponent("Transform")

	text_transform:setReference( box_transform )
	text_transform.scale.x = 1.0 / box_transform.scale.x
	text_transform.scale.y = 1.0 / box_transform.scale.y

	
	if color ~= nil then
		local box_metadata = box:getComponent("Metadata")
		box_metadata["color"] = color
		box:setComponent("Metadata", box_metadata)
	end

	local text_metadata = text:getComponent("Metadata")
	text_metadata["string"] = str
	text:setComponent("Metadata", text_metadata)
end

-- bind references
if children.name.box:uid() > 0 and children.name.string:uid() > 0 then
	bind( children.name.string, children.name.box, metadata["dialogue"]["name"] or "%name%", metadata["dialogue"]["name color"] )
end
if children.text.box:uid() > 0 and children.text.string:uid() > 0 then
	bind( children.text.string, children.text.box, metadata["dialogue"]["text"] or "%text%", metadata["dialogue"]["text color"] )
end

--[[
local destination = function( obj, x, y, z )
	local static = Static.get(obj)
	local transform = obj:getComponent("Transform")
	static.from = Vector3f(x or transform.position.x, y or transform.position.y, z or transform.position.z)
end
-- circleOut
destination(children.circleOut, nil, -2, 0)
destination(children.circleIn, nil, 2, 0)
destination(children.coverBar, -1.5, nil, 0)
destination(children.commandText, -1.5, nil, 0)
destination(children.tenkouseiOption, -1.5, nil, 0)
destination(children.closeOption, -1.5, nil, 0)
destination(children.quit, -1.5, nil, 0)
]]

--[[
local playSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
--	local assetLoader = scene:getComponent("Asset")
--	assetLoader:cache(soundEmitter:formatHookName("asset:Load.%UID%"), string.resolveURI(url), "")
end

ent:addHook("menu:Close.%UID%", function( json )
	playSound("menu close")
	if metadata["system"]["hooks"] == nil then metadata["system"]["hooks"] = {} end
	metadata["system"]["hooks"]["onClose"] = json["callback"];
	metadata["system"]["closing"] = true;
	ent:setComponent("Metadata", metadata)
end )

playSound("menu open")
]]

local states = {}

ent:bind( "tick", function(self)
	local closed = inputs.key("E") or inputs.key("R_Y")
	-- close when finished
	-- dialogue box
	if children.text.string:uid() > 0 then
		local child = children.text.string
		if states[child:uid()] == nil then
			states[child:uid()] = {
				timer = 0,
				finished = false,
				speed = -1
			}
		end
		local state = states[child:uid()]
		if state.finished then
			if closed then
				entities.destroy(self)
			end
		else
			local metadata = child:getComponent("Metadata")
			if state.speed < 0 then
				state.speed = metadata["speed"]
			end
			if metadata["range"] ~= nil and state.timer + state.speed < timer:elapsed() then
				local pos = metadata["range"][2]
				local str = metadata["string"]
				state.finished = not (pos < string.len(str))

				if pos < string.len(str) then
					metadata["range"][2] = pos + 1
					state.timer = timer:elapsed()

					child:setComponent("Metadata", metadata)
				end
			end
		end
	end
end )