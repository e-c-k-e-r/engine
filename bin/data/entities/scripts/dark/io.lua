local ent = ent
local scene = entities.currentScene()

local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local classTags = darkMeta["class_tags"] or ""
local connections = darkMeta["connections"]

local eName = ent:name() or tostring(ent:uid())
local eID = darkMeta["id"] or "??"

_G.DarkTargets = _G.DarkTargets or {}
if darkMeta["id"] then
	_G.DarkTargets[darkMeta["id"]] = ent:uid()
end

_G.OSM = _G.OSM or {}
_G.DarkUtils = _G.DarkUtils or {}

_G.DarkUtils.playSound = function(entity, tags, explicitSchema, emitOptions)
	local result = hooks.call("dark:ResolveSchema", { tags = tags, schema = explicitSchema })

	if result and result.wavs and #result.wavs > 0 then
		local pick = result.wavs[math.random(#result.wavs)]
		local meta = entity:getComponent("Metadata")
		local resolvedUrl = string.resolveURI(pick, meta["system"]["root"])

		local options = emitOptions or {}
		options.filename = resolvedUrl
		if options.spatial == nil then options.spatial = true end
		if options.volume == nil then options.volume = 1.0 end

		entity:callHook("sound:Emit.%UID%", options)
		return resolvedUrl, result.name
	end
	return nil, nil
end

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message
	local callerID = payload.callerDarkID or "??"

	--print(string.format("[I/O] %s (%s) RECEIVED '%s' from (%s)", eName, eID, msg, callerID))

	local scripts = darkMeta["scripts"] or {}
	for _, script in ipairs(scripts) do
		local scriptDef = _G.OSM[script]
		if scriptDef then
			--print(string.format("  -> Handling via script: %s", script))
			if type(scriptDef) == "table" and scriptDef.onMessage then
				scriptDef.onMessage(ent, payload, darkMeta)
			elseif type(scriptDef) == "function" then
				scriptDef(ent, payload, darkMeta)
			end
		end
	end
end)

ent:addHook("link:Broadcast.%UID%", function(payload)
	local msg = payload.message
	local validFlavors = payload.flavors or { "ControlDevice", "SwitchLink" }

	if not connections then
		--print(string.format("[I/O] %s (%s) tried to broadcast '%s', but has no connections.", eName, eID, msg))
		return
	end

	--print(string.format("[I/O] %s (%s) BROADCASTING '%s'", eName, eID, msg))

	for i = 1, #connections do
		local conn = connections[i]
		local isValid = false
		for _, flav in ipairs(validFlavors) do
			if conn.flavor == flav then isValid = true break end
		end

		if isValid then
			local targetUID = _G.DarkTargets[conn.target_id]
			if targetUID then
				--print(string.format("  -> Sent to target ID %s via %s", conn.target_id, conn.flavor))
				local targetEnt = entities.get(targetUID)
				if targetEnt and targetEnt:uid() then
					targetEnt:callHook("link:Message." .. targetUID, {
						message = msg,
						caller = ent:uid(),
						callerDarkID = darkMeta["id"] or payload.callerDarkID
					})
				end
			else
				--print(string.format("  -> Target ID %s not found in DarkTargets! (Flavor: %s)", conn.target_id, conn.flavor))
			end
		end
	end
end)


-- frobbage
if darkMeta["frob"] ~= nil then
	ent:addHook("entity:Use.%UID%", function(payload)
		local frobWorld = darkMeta["frob"]["world"] or 0
		local kFrobMove   = 1
		local kFrobScript = 2

		--print(string.format("[I/O] %s (%s) FROBBED by user.", eName, eID))

		if bit.band(frobWorld, kFrobScript) ~= 0 then
			local scripts = darkMeta["scripts"] or {}
			local handledByScript = false

			for _, script in ipairs(scripts) do
				if _G.OSM[script] and type(_G.OSM[script]) == "table" and _G.OSM[script].onFrob then
					--print(string.format("  -> Frob handled by script: %s", script))
					_G.OSM[script].onFrob(ent, payload, darkMeta)
					handledByScript = true
				end
			end

			if not handledByScript then
				--print("  -> Frob falling back to default TurnOn broadcast.")

				local played, schema = _G.DarkUtils.playSound(ent, classTags .. ", Event StateChange", nil, { spatial = true, maxDistance = 15.0 })
				if not played then played, schema = _G.DarkUtils.playSound(ent, classTags .. ", Event Activate", nil, { spatial = true, maxDistance = 15.0 }) end
				if not played then played, schema = _G.DarkUtils.playSound(ent, classTags, nil, { spatial = true, maxDistance = 15.0 }) end

				if played then
					--print(string.format("  -> Played fallback sound schema: %s", schema))
				end

				ent:callHook("link:Broadcast.%UID%", {
					message = "TurnOn",
					flavors = { "ControlDevice", "SwitchLink" },
					caller = payload.user,
					callerDarkID = darkMeta["id"]
				})
			end
		end

		if bit.band(frobWorld, kFrobMove) ~= 0 then
			--print(string.format("[I/O] %s (%s) Picked up!", eName, eID))
		end
	end)
end

ent:addHook("entity:Destroy.%UID%", function()
	if darkMeta["id"] and _G.DarkTargets[darkMeta["id"]] == ent:uid() then
		_G.DarkTargets[darkMeta["id"]] = nil
	end
end)