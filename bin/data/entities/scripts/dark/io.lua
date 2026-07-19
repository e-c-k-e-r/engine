local ent = ent
local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")

local FLAVOR_CONTROL_DEVICE = 1

_G.DarkTargets = _G.DarkTargets or {}
local darkMeta = metadata["dark"] or {}
if darkMeta["id"] then
	_G.DarkTargets[darkMeta["id"]] = ent:uid()
end

ent:addHook("dark:Message.%UID%", function(payload)
	local msg = payload.message
	local caller = payload.caller

	if msg == "TurnOn" then
		print("Dark I/O [".. ent:name() .. "]: Received TurnOn from " .. tostring(caller))
	elseif msg == "TurnOff" then
		print("Dark I/O [".. ent:name() .. "]: Received TurnOff from " .. tostring(caller))
	end
end)

local connections = metadata["connections"]
if not connections then return end

ent:addHook("dark:Broadcast.%UID%", function(payload)
	local msg = payload.message
	local validFlavors = payload.flavors or { "ControlDevice", "SwitchLink" }

	for i = 1, #connections do
		local conn = connections[i]
		local isValid = false
		for _, flav in ipairs(validFlavors) do
			if conn.flavor == flav then isValid = true break end
		end

		if isValid then
			local targetUID = _G.DarkTargets[conn.target_id]
			if targetUID then
				local targetEnt = entities.get(targetUID)
				if targetEnt and targetEnt:uid() then
					targetEnt:callHook("dark:Message." .. targetUID, {
						message = msg,
						caller = ent:uid()
					})
				end
			end
		end
	end
end)

ent:addHook("entity:Destroy.%UID%", function()
	if darkMeta["id"] and _G.DarkTargets[darkMeta["id"]] == ent:uid() then
		_G.DarkTargets[darkMeta["id"]] = nil
	end
end)

ent:addHook("entity:Use.%UID%", function(payload)
	if payload.user == ent:uid() then return end
	ent:callHook("dark:Broadcast.%UID%", { message = "TurnOn" })
end)