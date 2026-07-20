local ent = ent
local scene = entities.currentScene()

local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local schemaDb = darkMeta["schema_db"] or {}
local classTags = darkMeta["class_tags"] or ""

_G.DarkTargets = _G.DarkTargets or {}
if darkMeta["id"] then
	_G.DarkTargets[darkMeta["id"]] = ent:uid()
end

_G.DarkScripts = _G.DarkScripts or {}

_G.DarkScripts["RelayTrap"] = function(entity, payload, dMeta)
	local msg = payload.message
	local eName = entity or entity:uid()
	--print(string.format("%s (RelayTrap) is forwarding %s down its links...", eName, msg))

	entity:callHook("link:Broadcast.%UID%", {
		message = msg,
		flavors = { "ControlDevice", "SwitchLink" },
		caller = entity:uid(),
		callerDarkID = payload.callerDarkID
	})
end
_G.DarkScripts["ElevatorButton"] = _G.DarkScripts["RelayTrap"]

_G.DarkScripts["RequireAllTrap"] = function(entity, payload, dMeta)
	local msg = payload.message
	local callerDarkID = payload.callerDarkID

	_G.RAT_States = _G.RAT_States or {}
	local uid = entity:uid()
	_G.RAT_States[uid] = _G.RAT_States[uid] or { inputs = {}, wasOn = false }
	local state = _G.RAT_States[uid]

	if callerDarkID then
		state.inputs[callerDarkID] = (msg == "TurnOn")
	end

	local allOn = true
	local incoming = dMeta["incoming_connections"] or {}
	for i = 1, #incoming do
		local conn = incoming[i]
		if conn.flavor == "ControlDevice" or conn.flavor == "SwitchLink" then
			if not state.inputs[conn.source_id] then
				allOn = false
				break
			end
		end
	end

	if allOn and not state.wasOn then
		state.wasOn = true
		--print(entity .. " (RequireAllTrap) conditions met! Broadcasting TurnOn.")
		entity:callHook("link:Broadcast.%UID%", { message = "TurnOn", flavors = { "ControlDevice", "SwitchLink" }, callerDarkID = dMeta["id"], caller = uid })
	elseif not allOn and state.wasOn then
		state.wasOn = false
		--print(entity .. " (RequireAllTrap) condition lost! Broadcasting TurnOff.")
		entity:callHook("link:Broadcast.%UID%", { message = "TurnOff", flavors = { "ControlDevice", "SwitchLink" }, callerDarkID = dMeta["id"], caller = uid })
	end
end

--[[
_G.DarkScripts["BaseElevator"] = function(entity, payload, dMeta)
	local msg = payload.message
	local eName = entity or entity:uid()

	print(string.format("%s (Elevator) received command: %s", eName, msg))

	if msg == "TurnOn" or msg == "TurnOff" then
		print(string.format("--> Commanding Lift '%s' to move!", eName))
	end
end

_G.DarkScripts["Elevator"] = _G.DarkScripts["BaseElevator"]
]]

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message
	local caller = payload.caller
	local eName = ent:name() or ent:uid()

	--print(string.format("Entity %s (Dark ID: %s) received message: %s from UID: %s", eName, darkMeta["id"] or "Unknown", msg, tostring(caller)))

	local scripts = darkMeta["scripts"] or {}
	local scriptHandled = false

	for _, script in ipairs(scripts) do
		if _G.DarkScripts[script] then
			_G.DarkScripts[script](ent, payload, darkMeta)
			scriptHandled = true
		end
	end

	if not scriptHandled then
		if eName:lower():match("filter") or eName:lower():match("trigger") then
			_G.DarkScripts["RelayTrap"](ent, payload, darkMeta)
		end
	end
end)

local function playButtonSound()
	if not classTags or classTags == "" then return end
	local myDeviceType = string.match(classTags, "DeviceType%s+([^%s,]+)")

	local bestMatch = nil
	local bestScore = -1

	for _, schema in ipairs(schemaDb) do
		local sTags = schema.tags or ""
		local score = 0

		if myDeviceType then
            local paddedTags = sTags .. ","

            if string.find(paddedTags, "DeviceType%s+" .. myDeviceType .. "[,%s]") then
                score = score + 10
            elseif string.find(paddedTags, "DeviceType%s+Gen[,%s]") then
                score = score + 5
            elseif string.find(sTags, "DeviceType") then
                score = score - 10
            end

            if score > 0 then
                if string.find(sTags, "Reject") then
                    score = score + 5
                elseif string.find(sTags, "StateChange") or string.find(sTags, "Activate") then
                    score = score + 5
                end
            end
        end

		if score > bestScore then
			bestScore = score
			bestMatch = schema
		end
	end

	print("Button sound resolution:", ent, classTags, myDeviceType, bestMatch and bestMatch.name or "None", bestScore)

	if bestMatch and bestScore > 0 and bestMatch.wavs and #bestMatch.wavs > 0 then
		local pick = bestMatch.wavs[math.random(#bestMatch.wavs)]
		local resolvedUrl = string.resolveURI(pick, metadata["system"]["root"])
		ent:callHook("sound:Emit.%UID%", {
			filename = resolvedUrl, spatial = true, volume = 1.0, maxDistance = 15.0, rolloffFactor = 1.0, unique = true
		})
	end
end

local connections = darkMeta["connections"]

ent:addHook("link:Broadcast.%UID%", function(payload)
	local msg = payload.message
	local validFlavors = payload.flavors or { "ControlDevice", "SwitchLink" }

	if not connections then
		return
	end

	for i = 1, #connections do
		local conn = connections[i]
		local isValid = false
		for _, flav in ipairs(validFlavors) do
			if conn.flavor == flav then isValid = true break end
		end

		if isValid then
			local targetUID = _G.DarkTargets[conn.target_id]
			--print(string.format("Broadcasting %s to Dark ID %s (UID: %s) via %s", msg, conn.target_id, tostring(targetUID), conn.flavor))

			if targetUID then
				local targetEnt = entities.get(targetUID)
				if targetEnt and targetEnt:uid() then
					targetEnt:callHook("link:Message." .. targetUID, {
						message = msg,
						caller = ent:uid(),
						callerDarkID = darkMeta["id"] or payload.callerDarkID
					})
				end
			end
		end
	end
end)


if darkMeta["frob"] ~= nil then
	ent:addHook("entity:Use.%UID%", function(payload)
		local frobWorld = darkMeta["frob"]["world"] or 0

		local kFrobMove   = 1 -- 1 << 0
		local kFrobScript = 2 -- 1 << 1

		if bit.band(frobWorld, kFrobScript) ~= 0 then
			print(ent, json.encode( darkMeta ))
			playButtonSound()

			ent:callHook("link:Broadcast.%UID%", {
				message = "TurnOn",
				flavors = { "ControlDevice", "SwitchLink" },
				caller = payload.user,
				callerDarkID = darkMeta["id"]
			})
		end

		if bit.band(frobWorld, kFrobMove) ~= 0 then
			print("Item picked up!")
			-- ent:callHook("inventory:Pickup.%UID%", { user = payload.user })
		end
	end)
end

ent:addHook("entity:Destroy.%UID%", function()
	if darkMeta["id"] and _G.DarkTargets[darkMeta["id"]] == ent:uid() then
		_G.DarkTargets[darkMeta["id"]] = nil
	end

	if _G.RAT_States and _G.RAT_States[ent:uid()] then
		_G.RAT_States[ent:uid()] = nil
	end
end)