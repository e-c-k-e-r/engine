local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local startCount = tonumber(metadataValve["StartCount"]) or 0
local tripCount = tonumber(metadataValve["TripCount"]) or 10
local flags = tonumber(metadataValve["spawnflags"]) or 0

local enabled = (math.floor(flags / 1) % 2) == 0
local count = startCount

ent:addHook("io:Input.%UID%", function( payload )
	if not enabled then return end

	local input = payload.input

	if input == "Increment" then
		count = count + 1
		if count >= tripCount then
			ent:callHook("io:FireOutput.%UID%", { output = "OnHigh" })
			count = startCount
		end
	elseif input == "Decrement" then
		count = count - 1
		if count < startCount then
			ent:callHook("io:FireOutput.%UID%", { output = "OnLow" })
			count = startCount
		end
	elseif input == "Reset" then
		count = startCount
	elseif input == "Set" then
		local value = tonumber(payload.parameter)
		if value then
			count = math.floor(value)
		end
	end
end)
