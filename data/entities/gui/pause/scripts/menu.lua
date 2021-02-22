local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local masterdata = scene:getComponent("Metadata")

local children = {
	mainText = ent:loadChild("./main-text.json",true),
	circleOut = ent:loadChild("./circle-out.json",true),
	circleIn = ent:loadChild("./circle-in.json",true),
	coverBar = ent:loadChild("./yellow-box.json",true),
	commandText = ent:loadChild("./command-text.json",true),
	tenkouseiOption = ent:loadChild("./tenkousei.json",true),
	closeOption = ent:loadChild("./close.json",true),
	quit = ent:loadChild("./quit.json",true),
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

local playSound = function( key )
	local url = "/ui/" .. key .. ".ogg"
	local assetLoader = scene:getComponent("Asset")
	assetLoader:cache(string.resolveURI(url), soundEmitter:formatHookName("asset:Load.%UID%"), "")
end

ent:addHook("menu:Close.%UID%", function( json )
	playSound("menu close")

	metadata["system"]["hooks"]["onClose"] = json["callback"];
	metadata["system"]["closing"] = true;
	ent:setComponent("Metadata", metadata)
end )

--[[
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
]]
playSound("menu open")

ent:bind( "tick", function(self)
	local static = Static.get(self)
	if not static.alpha then
		static.alpha = 0
	end

	-- handle closing
	if metadata["system"]["closing"] then
		if static.alpha <= 0 then
			static.alpha = 0
			metadata["system"]["closing"] = false
			metadata["system"]["closed"] = true
		else
			static.alpha = static.alpha - time.delta()
		end
	elseif metadata["system"]["closed"] then
		timer:stop()
		local callback = metadata["system"]["hooks"]["onClose"]
		if callback then
			local payload = callback["payload"]
			local target = self
			if callback["scope"] == "parent" then
				target = ent:getParent()
			elseif callback["scope"] == "scene" then
				target = scene
			end

			if type(callback["delay"]) == "number" and target:uid() ~= self:uid() then
				target:queueHook( callback["name"], payload, callback["delay"] );
			else
				target:callHook( callback["name"], payload );
			end
		end
		entities.destroy(self)
		-- scene:queueHook("system:Destroy", { uid = self:uid() }, 0)
		return
	else
		if not metadata["initialized"] then
			static.alpha = 0
		end
		metadata["initialized"] = true;
		if  static.alpha >= 1.0 then
			static.alpha = 1.0
		else
			static.alpha = static.alpha + time.delta() * 1.5
		end
	end

	metadata["alpha"] = static.alpha;
	self:setComponent("Metadata", metadata)
	-- set alphas
	for k, v in pairs(children) do
		if v:uid() <= 0 then goto continue end

		-- set alpha
		local metadata = v:getComponent("Metadata")
		metadata["alpha"] = static.alpha
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

	-- main text
	local child = children.mainText
	if child:uid() > 0 then
		local transform = child:getComponent("Transform")
		local metadata = child:getComponent("Metadata")
		local speed = metadata["hovered"] and 0.75 or 0.5
		transform.position.y = transform.position.y + time.delta() * speed
		if transform.position.y > 2 then
			transform.position.y = -2
		end
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

	if window.keyPressed("Escape") and timer:elapsed() >= 1 then
		timer:reset()
		self:callHook("menu:Close.%UID%", {})
	end
end )