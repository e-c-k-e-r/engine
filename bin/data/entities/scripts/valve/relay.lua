local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local timer = Timer.new()
if not timer:running() then
	timer:start()
end

local delay = tonumber(metadataValve["delay"]) or 0.0
local enabled = true

local pendingOutputs = {}

ent:addHook("io:Input.%UID%", function( payload )
	local input = payload.input

	if input == "Enable" then
		enabled = true
	elseif input == "Disable" then
		enabled = false
	elseif input == "Toggle" then
		enabled = not enabled
	else
		if not enabled then return end

		table.insert(pendingOutputs, {
			fireTime = timer:elapsed() + math.max(0.0, delay),
			input = input,
			parameter = payload.parameter
		})
	end
end)

ent:bind( "tick", function(self)
	for i = #pendingOutputs, 1, -1 do
		local job = pendingOutputs[i]
		if timer:elapsed() >= job.fireTime then
			ent:queueHook("io:FireOutput.%UID%", { output = job.input, parameter = job.parameter }, 0)
			table.remove(pendingOutputs, i)
		end
	end
end )
