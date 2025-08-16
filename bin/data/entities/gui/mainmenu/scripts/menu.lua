Static = {
	values = {},
	get = function( obj )
		if Static.values[""..obj:uid()] == nil then
			Static.values[""..obj:uid()] = {}
		end
		return Static.values[""..obj:uid()]
	end
}

local ent = ent
local scene = entities.currentScene()
local controller = entities.controller()
local camera = controller:getComponent("Camera")
local metadata = ent:getComponent("Metadata")
local masterdata = scene:getComponent("Metadata")

local soundEmitter = ent:loadChild("./sound.json",true)

local children = {
	mainText = ent:loadChild("./main-text.json",true),
	circleOut = ent:loadChild("./circle-out.json",true),
	circleIn = ent:loadChild("./circle-in.json",true),
	start = ent:loadChild("./start.json",true),
	quit = ent:loadChild("./quit.json",true),
}

local timer = Timer.new()
if not timer:running() then timer:start() end

local playSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
	soundEmitter:callHook("sound:Emit.%UID%", { filename = url })
--	local assetLoader = scene:getComponent("Asset")
--	assetLoader:cache(ent:formatHookName("asset:Load.%UID%"), string.resolveURI(url))
end
local destination = function( obj, x, y, z )
	local static = Static.get(obj)
	local transform = obj:getComponent("Transform")
	static.from = Vector3f(x or transform.position.x, y or transform.position.y, z or transform.position.z)
end

--[[
pcall( function()
	local metadata = controller:getComponent("Metadata")
	local json = json.readFromFile("./data/entities/player.json");
	controller:callHook("object:UpdateMetadata.%UID%", {
		overlay = json["metadata"]["overlay"]
	})
	camera:update(true);
end )
]]

destination(children.mainText, nil, -2, 0)
destination(children.circleOut, nil, -2, 0)
destination(children.circleIn, nil, 2, 0)
destination(children.start, -1.5, nil, 0)
destination(children.quit, -1.5, nil, 0)

ent:addHook("asset:Load.%UID%", function( json )
	local filename = json["filename"]
	if filename == "" or string.extension( filename ) ~= "ogg" then return false end

	local sfx = ent:getComponent("Audio")
	if not sfx:playing() then sfx:stop() end
	sfx:load( filename )
	sfx:setVolume( masterdata["volumes"]["sfx"] )
	sfx:play()

	return true
end )

local selectableElements = {
	children.start,
	children.quit,
}
local selectableElementColors = {}
local selectedElement = 0
local selectionColor = { 1, 0, 1, 1 }
local INPUT_DELAY = 0.2

local function array_index_of( haystack, needle )
	for i, v in ipairs(haystack) do
		if v == needle then
			return i
		end
	end
	return 0
end
local function onHover( payload )
	playSound( "buttonrollover" )
	selectedElement = 0
end
local function onClick( payload )
	playSound( "buttonclickrelease" )
end

for i, v in ipairs( selectableElements ) do
	selectableElementColors[i] = nil
	v:addHook("gui:HoverStart.%UID%", onHover )
	v:addHook("gui:ClickStart.%UID%", onClick )
end

local function handleSelectionIndex()
	if (inputs.key("L_DPAD_UP") or inputs.key("Up")) and timer:elapsed() >= INPUT_DELAY then
		timer:reset()
		-- nothing is selected
		if selectedElement == 0 then
			-- set to bottom
			selectedElement = #selectableElements
		else
			selectedElement = selectedElement - 1
			-- wrap to bottom
			if selectedElement <= 0 then
				selectedElement = #selectableElements
			end
		end
		playSound( "buttonrollover" )
	end
	if (inputs.key("L_DPAD_DOWN") or inputs.key("Down")) and timer:elapsed() >= INPUT_DELAY then
		timer:reset()
		-- nothing is selected
		if selectedElement == 0 then
			-- set to top
			selectedElement = 1
		else
			selectedElement = selectedElement + 1
			-- wrap to top
			if selectedElement > #selectableElements then
				selectedElement = 1
			end
		end
		playSound( "buttonrollover" )
	end
end

ent:bind( "tick", function(self)
	handleSelectionIndex()

	local static = Static.get(self)
	if not static.alpha then
		static.alpha = 0
	end

	metadata["initialized"] = true;

	if  static.alpha >= 1.0 then
		static.alpha = 1.0
	else
		static.alpha = static.alpha + time.delta() * 1.5
	end

	-- make background glow
	local glow = 1 + math.sin(1.25 * time.current()) * 0.125
	metadata["color"] = { glow, glow, glow, static.alpha }
	self:setComponent("Metadata", metadata)

	camera:update(true);

	-- iterate children in batch
	for k, v in pairs(children) do
		if v:uid() <= 0 then goto continue end

		-- set alpha
		local metadata = v:getComponent("Metadata")
		local index = array_index_of( selectableElements, v )
		-- mark element as hovered if selected
		if 0 < selectedElement and selectedElement <= #selectableElements then
			if 0 < index and index <= #selectableElements then
				metadata["hovered"] = index == selectedElement
			end
		end

		if metadata["clickable"] then
			-- backup color
			if selectableElementColors[index] == nil then
				selectableElementColors[index] = metadata["color"]
			end

			-- color for selection
			if metadata["hovered"] then
				metadata["color"] = selectionColor
				-- simulate click on input press
				if (inputs.key("A") or inputs.key("START") or inputs.key("Enter")) and timer:elapsed() >= INPUT_DELAY then
					timer:reset()
					v:callHook("gui:Clicked.%UID%", {})
				end
			else
				metadata["color"] = selectableElementColors[index]
			end
		end

		metadata["color"][4] = static.alpha
		v:setComponent("Metadata", metadata)

		local transform = v:getComponent("Transform")
		local static = Static.get(v)
		-- translation
		if not static.from then goto continue end
		if not static.to then static.to = Vector3f(transform.position) end
		if not static.delta then static.delta = 0 end

		if static.delta >= 1 then
			static.delta = 1
		else
			static.delta = static.delta + time.delta() * 1.5
			transform.position = Vector3f.lerp( static.from, static.to, static.delta )
		end


		::continue::
	end	

	-- circle in
	child = children.circleIn
	if child:uid() > 0 then
		local static = Static.get( child )

		local transform = child:getComponent("Transform")
		local metadata = child:getComponent("Metadata")

		-- rotation
		local speed = metadata["hovered"] and 0.25 or 0.0125
		static.time = (static.time or 0) + time.delta() * -speed
		transform.orientation = Quaternion.axisAngle( Vector3f(0, 0, 1), static.time )
	end
	-- circle out
	child = children.circleOut
	if child:uid() > 0 then
		local static = Static.get( child )

		local transform = child:getComponent("Transform")
		local metadata = child:getComponent("Metadata")

		-- rotation
		local speed = metadata["hovered"] and 0.25 or 0.0125
		static.time = (static.time or 0) + time.delta() * speed
		transform.orientation = Quaternion.axisAngle( Vector3f(0, 0, 1), static.time )
	end
end )