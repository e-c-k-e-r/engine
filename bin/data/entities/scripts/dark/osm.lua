_G.OSM = _G.OSM or {}
_G.DarkUtils = _G.DarkUtils or {}
_G.DarkQB = _G.DarkQB or {}

_G.OSM_States = _G.OSM_States or {}
local function getState( entity )
	local uid = entity:uid()
	_G.OSM_States[uid] = _G.OSM_States[uid] or {}
	return _G.OSM_States[uid]
end

-- play a frob/activate sound for an object, falling back through tag queries
local function playActivateSound( entity, dMeta )
	local tags = dMeta["class_tags"] or ""
	if tags ~= "" then tags = tags .. ", " end
	local played = _G.DarkUtils.playSound( entity, tags .. "Event Activate", nil, { spatial = true, maxDistance = 15.0 } )
	if not played then played = _G.DarkUtils.playSound( entity, tags .. "Event StateChange", nil, { spatial = true, maxDistance = 15.0 } ) end
	if not played then _G.DarkUtils.playSound( entity, dMeta["class_tags"] or "", nil, { spatial = true, maxDistance = 15.0 } ) end
end

local function broadcast( entity, dMeta, message, caller, flavors )
	entity:callHook( "link:Broadcast.%UID%", {
		message = message,
		flavors = flavors or { "ControlDevice", "SwitchLink" },
		caller = caller or entity:uid(),
		callerDarkID = dMeta["id"]
	})
end

-- resolve a dark object id to an entity
local function getTarget( darkID )
	local targetUID = _G.DarkTargets and _G.DarkTargets[darkID]
	if not targetUID then return nil end
	local targetEnt = entities.get(targetUID)
	if targetEnt and targetEnt:uid() then return targetEnt end
	return nil
end

-- find the first connection matching a flavor substring, returns (connection, target entity)
local function findConnection( dMeta, flavorMatch )
	local conns = dMeta["connections"] or {}
	for i = 1, #conns do
		local conn = conns[i]
		if string.find( conn.flavor or "", flavorMatch, 1, true ) then
			return conn, getTarget( conn.target_id )
		end
	end
	return nil, nil
end

-- per-object state cleanup
local ent = ent
ent:addHook( "entity:Destroy.%UID%", function()
	if _G.OSM_States[ent:uid()] then _G.OSM_States[ent:uid()] = nil end
end)

-- relays / logic
_G.OSM["RelayTrap"] = function(entity, payload, dMeta)
	local msg = payload.message
	broadcast( entity, dMeta, msg, entity:uid(), { "ControlDevice", "SwitchLink" } )
end

-- forwards any message it receives (including Toggle)
_G.OSM["TrapRouter"] = _G.OSM["RelayTrap"]

-- relays the first message it receives, then goes inert
_G.OSM["OnceRouter"] = function(entity, payload, dMeta)
	local st = getState( entity )
	if st.onceFired then return end
	st.onceFired = true
	broadcast( entity, dMeta, payload.message, entity:uid(), { "ControlDevice", "SwitchLink" } )
end

-- relays a message after a delay (seconds; override with dark metadata "delay")
_G.OSM["TrapDelay"] = function(entity, payload, dMeta)
	local msg = payload.message
	if not msg then return end
	local delay = tonumber( dMeta["delay"] ) or 2.0
	entity:queueHook( "link:Broadcast.%UID%", {
		message = msg,
		flavors = { "ControlDevice", "SwitchLink" },
		caller = entity:uid(),
		callerDarkID = dMeta["id"]
	}, delay )
end

-- quest bit (QB) helpers. Bits are keyed by the dark id of the object that
-- set them; filters gate on the bits of their incoming sources.
local function qbSet( key, value )
	if value then _G.DarkQB[key] = true else _G.DarkQB[key] = nil end
end

local function qbSatisfied( dMeta, negative )
	local incoming = dMeta["incoming_connections"] or {}
	if #incoming == 0 then return true end -- fail open if unwired
	for i = 1, #incoming do
		local conn = incoming[i]
		local key = tostring( dMeta["qb"] or conn.source_id )
		if _G.DarkQB[key] then return not negative end
	end
	return negative
end

-- sets a quest bit when triggered, then relays the message
_G.OSM["TrapQBSet"] = function(entity, payload, dMeta)
	local msg = payload.message
	local key = tostring( dMeta["id"] )
	if msg == "TurnOn" then qbSet( key, true )
	elseif msg == "TurnOff" then qbSet( key, false ) end
	broadcast( entity, dMeta, msg, entity:uid(), { "ControlDevice", "SwitchLink" } )
end

-- sets a quest bit when frobb'd
_G.OSM["FrobQB"] = {
	onFrob = function(entity, payload, dMeta)
		qbSet( tostring( dMeta["id"] ), true )
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, "TurnOn", payload.user )
	end
}

-- only relays while its quest bit is set
_G.OSM["TrapQBFilter"] = function(entity, payload, dMeta)
	if not qbSatisfied( dMeta, false ) then return end
	broadcast( entity, dMeta, payload.message, entity:uid(), { "ControlDevice", "SwitchLink" } )
end

-- only relays while its quest bit is unset
_G.OSM["TrapQBNegFilter"] = function(entity, payload, dMeta)
	if not qbSatisfied( dMeta, true ) then return end
	broadcast( entity, dMeta, payload.message, entity:uid(), { "ControlDevice", "SwitchLink" } )
end

-- relays only once all of its control inputs are on
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

-- buttons / tweqs
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

-- basic button: frob turns it on (one-shot)
_G.OSM["BaseButton"] = {
	onFrob = function(entity, payload, dMeta)
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, "TurnOn", payload.user )
	end
}

-- toggle button: alternates TurnOn / TurnOff on each frob
_G.OSM["TwoStateButton"] = {
	onFrob = function(entity, payload, dMeta)
		local st = getState( entity )
		st.isOn = not st.isOn
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, st.isOn and "TurnOn" or "TurnOff", payload.user )
	end
}

-- depressable tweq: frob latches it on
_G.OSM["TweqDepressable"] = {
	onFrob = function(entity, payload, dMeta)
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, "TurnOn", payload.user )
	end
}

-- toggle tweq: frob or message flips its state and relays it
_G.OSM["TrapTweq"] = {
	onMessage = function(entity, payload, dMeta)
		local st = getState( entity )
		st.isOn = (payload.message == "TurnOn")
		broadcast( entity, dMeta, payload.message, payload.caller or entity:uid(), { "ControlDevice", "SwitchLink" } )
	end,

	onFrob = function(entity, payload, dMeta)
		local st = getState( entity )
		st.isOn = not st.isOn
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, st.isOn and "TurnOn" or "TurnOff", payload.user )
	end
}

-- doors
-- standard door: frob toggles it open/closed (door.lua performs the motion)
_G.OSM["StdDoor"] = {
	onMessage = function(entity, payload, dMeta)
		local st = getState( entity )
		if payload.message == "TurnOn" then st.isOpen = true
		elseif payload.message == "TurnOff" then st.isOpen = false end
	end,

	onFrob = function(entity, payload, dMeta)
		local st = getState( entity )
		if st.isOpen == nil then
			st.isOpen = (tonumber( (dMeta["door"] or {})["status"] ) == 1)
		end
		st.isOpen = not st.isOpen
		playActivateSound( entity, dMeta )
		broadcast( entity, dMeta, st.isOpen and "TurnOn" or "TurnOff", payload.user )
	end
}

-- lights
-- find the light child node created by the level loader ("{name}_light")
local function getLightComponent( entity )
	local lightEnt = entity:findByName( (entity:name() or "") .. "_light" )
	if not lightEnt then return nil end
	return lightEnt:getComponent("LightBehavior::Metadata")
end

local function setLightPower( entity, on )
	local st = getState( entity )
	local lightComp = getLightComponent( entity )
	if not lightComp then return end
	if st.lightPower == nil then st.lightPower = lightComp.power end
	lightComp.power = on and st.lightPower or 0.0
end

-- switchable light: TurnOn/TurnOff (or frob) toggles its power
_G.OSM["BaseLight"] = {
	onMessage = function(entity, payload, dMeta)
		if payload.message == "TurnOn" then setLightPower( entity, true )
		elseif payload.message == "TurnOff" then setLightPower( entity, false ) end
	end,

	onFrob = function(entity, payload, dMeta)
		local st = getState( entity )
		st.isOn = not st.isOn
		playActivateSound( entity, dMeta )
		setLightPower( entity, st.isOn )
		broadcast( entity, dMeta, st.isOn and "TurnOn" or "TurnOff", payload.user )
	end
}

-- light that hums while it is on
_G.OSM["LightSoundOn"] = {
	onMessage = function(entity, payload, dMeta)
		if payload.message == "TurnOn" then
			setLightPower( entity, true )
			local st = getState( entity )
			if not st.isSounding then
				local soundMeta = dMeta["sound"] or {}
				local explicitSchema = soundMeta["schema"] or ""
				local played = _G.DarkUtils.playSound( entity, "", explicitSchema, { spatial = true, loop = true, unique = true } )
				if not played then
					played = _G.DarkUtils.playSound( entity, (dMeta["class_tags"] or "") .. ", Event Activate", nil, { spatial = true, loop = true, unique = true } )
				end
				st.isSounding = played ~= nil
			end
		elseif payload.message == "TurnOff" then
			setLightPower( entity, false )
			local st = getState( entity )
			if st.isSounding then
				st.isSounding = nil
				entity:callHook( "sound:Stop.%UID%", {} )
			end
		end
	end,

	onFrob = function(entity, payload, dMeta)
		local st = getState( entity )
		st.isOn = not st.isOn
		playActivateSound( entity, dMeta )
		setLightPower( entity, st.isOn )
		broadcast( entity, dMeta, st.isOn and "TurnOn" or "TurnOff", payload.user )
	end
}

-- gravity zones
local function gravityTick( self, factor )
	local body = self:getComponent("PhysicsBody")
	if not body or not body:initialized() then return end

	local st = getState( self )
	local affected = st.gravAffected or {}
	st.gravAffected = affected
	local current = {}

	local events = body:getCollisionEvents()
	for i = 1, #events do
		local event = events[i]
		local otherBody
		if event.a:getObject():uid() == self:uid() then
			otherBody = event.b
		elseif event.b:getObject():uid() == self:uid() then
			otherBody = event.a
		end

		if otherBody then
			local otherEnt = otherBody:getObject()
			local uid = otherEnt:uid()
			if uid ~= self:uid() and otherBody:getMass() > 0.0 then
				current[uid] = true
				if not affected[uid] then
					otherBody:setGravity( Vector3f( 0, -9.81 * factor, 0 ) )
					affected[uid] = true
				end
			end
		end
	end

	for uid, _ in pairs( affected ) do
		if not current[uid] then
			local otherEnt = entities.get(uid)
			if otherEnt and otherEnt:uid() then
				local otherBody = otherEnt:getComponent("PhysicsBody")
				if otherBody and otherBody:initialized() then
					otherBody:enableGravity( true )
				end
			end
			affected[uid] = nil
		end
	end
end

-- destruction / teleport / message traps
local function destroyTick( self )
	local body = self:getComponent("PhysicsBody")
	if not body or not body:initialized() then return end

	local events = body:getCollisionEvents()
	for i = 1, #events do
		local event = events[i]
		local otherBody
		if event.a:getObject():uid() == self:uid() then
			otherBody = event.b
		elseif event.b:getObject():uid() == self:uid() then
			otherBody = event.a
		end

		if otherBody then
			local otherEnt = otherBody:getObject()
			if otherEnt:name() ~= "Player" and otherBody:getMass() > 0.0 then
				entities.destroy( otherEnt )
			end
		end
	end
end

-- destroys dynamic objects that touch it
_G.OSM["TriggerDestroy"] = function(entity, payload, dMeta) end
_G.OSM["TrapDestroyer"] = _G.OSM["TriggerDestroy"]
_G.OSM["TrapDestroy"] = _G.OSM["TriggerDestroy"]

-- on trigger, destroys every object whose name matches its target connection
local function destroyAllMatching( entity, dMeta )
	local conn, _ = findConnection( dMeta, "" )
	if not conn or not conn.target_node then return end
	local pattern = conn.target_node

	for i, victim in ipairs( entities.all() ) do
		if victim and victim:uid() and victim:uid() ~= entity:uid() and victim:name() == pattern then
			entities.destroy( victim )
		end
	end
end

_G.OSM["TrapTerminator"] = {
	onMessage = function(entity, payload, dMeta)
		if payload.message == "TurnOn" or payload.message == "Toggle" then
			destroyAllMatching( entity, dMeta )
		end
	end,
	onFrob = function(entity, payload, dMeta)
		playActivateSound( entity, dMeta )
		destroyAllMatching( entity, dMeta )
	end
}

_G.OSM["DestroyAllByName"] = _G.OSM["TrapTerminator"]

-- teleports the player to its teleport target on trigger or frob
local function teleportPlayer( entity, dMeta )
	local conn, targetEnt = findConnection( dMeta, "Tele" )
	if not targetEnt then return end

	local player = entities.controller()
	if not player or not player:uid() then return end

	local playerTransform = player:getComponent("Transform")
	local targetTransform = targetEnt:getComponent("Transform")
	playerTransform.position = targetTransform.position
	playerTransform.orientation = targetTransform.orientation
end

_G.OSM["TrapTeleport"] = {
	onMessage = function(entity, payload, dMeta)
		if payload.message == "TurnOn" or payload.message == "Toggle" then
			teleportPlayer( entity, dMeta )
		end
	end,
	onFrob = function(entity, payload, dMeta)
		playActivateSound( entity, dMeta )
		teleportPlayer( entity, dMeta )
	end
}

-- flashes a text message to the player on trigger or frob
_G.OSM["TrapMessage"] = {
	onMessage = function(entity, payload, dMeta)
		local text = dMeta["message"] or (entity:name() or "")
		print( "[OSM] TrapMessage: " .. tostring(text) )
		entity:callHook( "ui:FlashMessage", { text = text } )
	end,
	onFrob = function(entity, payload, dMeta)
		local text = dMeta["message"] or (entity:name() or "")
		print( "[OSM] TrapMessage: " .. tostring(text) )
		entity:callHook( "ui:FlashMessage", { text = text } )
	end
}

-- elevators
_G.OSM["ElevatorButton"] = _G.OSM["RelayTrap"]
_G.OSM["RerouteElevatorButton"] = _G.OSM["RelayTrap"]

_G.OSM["BaseElevator"] = function(entity, payload, dMeta)
	local msg = payload.message
	local eName = entity:name() or entity:uid()

	-- print(string.format("%s (Elevator) received command: %s", eName, msg))

	if msg == "TurnOn" or msg == "TurnOff" then
		-- print(string.format("--> Commanding Lift '%s' to move!", eName))
	end
end

_G.OSM["Elevator"] = _G.OSM["BaseElevator"]
_G.OSM["OldStyleBaseElevator"] = _G.OSM["BaseElevator"]

-- case variants found in level data (to-do: case insensitive mappings?)
_G.OSM["basebutton"] = _G.OSM["BaseButton"]
_G.OSM["trapdelay"] = _G.OSM["TrapDelay"]
_G.OSM["triggerdestroy"] = _G.OSM["TriggerDestroy"]
_G.OSM["Trapterminator"] = _G.OSM["TrapTerminator"]
_G.OSM["LIghtSoundOn"] = _G.OSM["LightSoundOn"]
_G.OSM["tweqtrap"] = _G.OSM["TrapTweq"]
_G.OSM["tweqbutton"] = _G.OSM["BaseButton"]
_G.OSM["Tweqable"] = _G.OSM["TrapTweq"]

-- per-object tick binding (only for scripts that need one)
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local myScripts = darkMeta["scripts"] or {}

for i = 1, #myScripts do
	local script = myScripts[i]
	if script == "TrapGravity" or script == "ZeroGravRoom" then
		local factor = tonumber( darkMeta["gravity"] )
		if factor == nil then factor = 0.0 end
		ent:bind( "tick", function(self) gravityTick( self, factor ) end )
	elseif script == "TriggerDestroy" or script == "triggerdestroy" or script == "TrapDestroyer" or script == "TrapDestroy" then
		ent:bind( "tick", function(self) destroyTick( self ) end )
	end
end
