local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

local refireTime = tonumber(metadataValve["RefireTime"]) or 10.0
local randomRange = tonumber(metadataValve["random"]) or 0.0
local flags = tonumber(metadataValve["spawnflags"]) or 0

local startDisabled = (tonumber(metadataValve["StartDisabled"]) or 0) ~= 0
if not startDisabled and (math.floor(flags / 1) % 2) ~= 0 then
	startDisabled = true
end

local enabled = not startDisabled
local nextFire = refireTime

local function interval()
	local value = refireTime
	if randomRange > 0 then
		value = value + (math.random() * 2.0 - 1.0) * randomRange
	end
	return math.max(0.01, value)
end

ent:bind( "tick", function(self)
	if not enabled then return end

	if timer:elapsed() >= nextFire then
		ent:queueHook("io:FireOutput.%UID%", { output = "OnTimer" }, 0)
		nextFire = timer:elapsed() + interval()
	end
end )

ent:addHook("io:Input.%UID%", function( payload )
	local input = payload.input

	if input == "Enable" then
		enabled = true
		nextFire = timer:elapsed() + refireTime
	elseif input == "Disable" then
		enabled = false
	elseif input == "Toggle" then
		enabled = not enabled
		if enabled then
			nextFire = timer:elapsed() + refireTime
		end
	elseif input == "Reset" then
		nextFire = timer:elapsed() + refireTime
	end
end)
