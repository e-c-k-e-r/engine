_G.OSM = _G.OSM or {}

_G.OSM["TweqLockedButton"] = {
	onMessage = function(entity, payload, dMeta) end,

	onFrob = function(entity, payload, dMeta)
		local eName = entity:name() or entity:uid()
		-- todo: deduce lock state
		local isLocked = true

		print(entity, "is locked?", isLocked)

		if isLocked then
			-- print(string.format("%s is locked! Emitting 'cardfail'.", eName))
			_G.DarkUtils.playSound(entity, "", "cardfail", { spatial = true, maxDistance = 15.0 })
			-- entity:callHook("ui:FlashMessage", { text = "Access Required: {}" })
		else
			-- unlock logic
			local cTags = (dMeta["class_tags"] or "") .. ", Event StateChange"
			_G.DarkUtils.playSound(entity, cTags, "", { spatial = true, maxDistance = 15.0 })

			entity:callHook("link:Broadcast.%UID%", {
				message = "TurnOn", flavors = { "ControlDevice" }, caller = payload.user, callerDarkID = dMeta["id"]
			})
		end
	end
}


_G.OSM["RelayTrap"] = function(entity, payload, dMeta)
	local msg = payload.message
	entity:callHook("link:Broadcast.%UID%", {
		message = msg,
		flavors = { "ControlDevice", "SwitchLink" },
		caller = entity:uid(),
		callerDarkID = payload.callerDarkID
	})
end

_G.OSM["TrapQBFilter"] = _G.OSM["RelayTrap"]
_G.OSM["TrapQBNegFilter"] = _G.OSM["RelayTrap"]
_G.OSM["ElevatorButton"] = _G.OSM["RelayTrap"]

_G.OSM["RequireAllTrap"] = function(entity, payload, dMeta)
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
		entity:callHook("link:Broadcast.%UID%", { message = "TurnOn", flavors = { "ControlDevice", "SwitchLink" }, callerDarkID = dMeta["id"], caller = uid })
	elseif not allOn and state.wasOn then
		state.wasOn = false
		entity:callHook("link:Broadcast.%UID%", { message = "TurnOff", flavors = { "ControlDevice", "SwitchLink" }, callerDarkID = dMeta["id"], caller = uid })
	end
end

_G.OSM["BaseElevator"] = function(entity, payload, dMeta)
	local msg = payload.message
	local eName = entity:name() or entity:uid()

	-- print(string.format("%s (Elevator) received command: %s", eName, msg))

	if msg == "TurnOn" or msg == "TurnOff" then
		-- print(string.format("--> Commanding Lift '%s' to move!", eName))
	end
end

_G.OSM["Elevator"] = _G.OSM["BaseElevator"]