local ent = ent
local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")

-- assign targets
_G.IOTargets = _G.IOTargets or {}

local metadataValve = metadata["valve"] or {}
if metadataValve["targetname"] then
	local tname = metadataValve["targetname"]
	if not _G.IOTargets[tname] then
		_G.IOTargets[tname] = {}
	end

	local uid = ent:uid()
	local exists = false
	for _, id in ipairs(_G.IOTargets[tname]) do
		if id == uid then exists = true break end
	end
	if not exists then
		table.insert(_G.IOTargets[tname], uid)
	end
end

ent:addHook("io:Input.%UID%", function(payload)
	local input = payload.input
	local param = payload.parameter

	-- kill
	if input == "Kill" or input == "KillHierarchy" then
		entities.destroy(ent)
	-- relays
	elseif input == "FireUser1" then
		ent:callHook("io:FireOutput.%UID%", { output = "OnUser1" })
	elseif input == "FireUser2" then
		ent:callHook("io:FireOutput.%UID%", { output = "OnUser2" })
	elseif input == "FireUser3" then
		ent:callHook("io:FireOutput.%UID%", { output = "OnUser3" })
	elseif input == "FireUser4" then
		ent:callHook("io:FireOutput.%UID%", { output = "OnUser4" })
    -- colors
	elseif input == "Alpha" then
		local alphaVal = tonumber(param) / 255.0
		if alphaVal then
			-- to-do: apply to instance / material
		end
	elseif input == "Color" then
		local r, g, b = param:match("(%d+) (%d+) (%d+)")
		-- to-do: apply to instance / material
	else
	   --print("I/O [(".. ent:uid() ..") " .. ent:name() .. "]: Received Input '" .. input .. "' with param '" .. tostring(param) .. "'")
	end
end )

-- no connections defined, bail early
local connections = metadata["connections"]
if not connections then return end

-- define connections
local timer = Timer.new()
if not timer:running() then
	timer:start()
end
local timesFired = {}
for i = 1, #connections do
	timesFired[i] = 0
end

local pendingOutputs = {}

ent:bind("tick", function(self)
	local currentTime = timer:elapsed()

	for i = #pendingOutputs, 1, -1 do
		local job = pendingOutputs[i]
		if currentTime >= job.fireTime then

			local targetUIDs = _G.IOTargets[job.target]
			if targetUIDs then
				for _, targetUID in ipairs(targetUIDs) do
					local targetEnt = entities.get(targetUID)
					if targetEnt:uid() then
						targetEnt:callHook("io:Input." .. targetUID, {
							input = job.input,
							parameter = job.parameter,
							caller = ent:uid()
						})
					end
				end
			else
				print("I/O Warning: Targetname '" .. job.target .. "' not found!")
			end

			table.remove(pendingOutputs, i)
		end
	end
end)

ent:addHook("io:FireOutput.%UID%", function(payload)
	local outputName = payload.output

	for i = 1, #connections do
		local conn = connections[i]

		if conn.output == outputName then
			local limit = conn.times or -1

			if limit == -1 or timesFired[i] < limit then
				timesFired[i] = timesFired[i] + 1

				local delay = conn.delay or 0.0
				table.insert(pendingOutputs, {
					fireTime = timer:elapsed() + delay,
					target = conn.target,
					input = conn.input,
					parameter = conn.parameter
				})
			end
		end
	end
end)

ent:addHook("entity:Destroy.%UID%", function()
	if metadataValve["targetname"] then
		local tname = metadataValve["targetname"]
		if _G.IOTargets[tname] then
			for i, id in ipairs(_G.IOTargets[tname]) do
				if id == ent:uid() then
					table.remove(_G.IOTargets[tname], i)
					break
				end
			end
		end
	end
end)

ent:addHook("entity:Use.%UID%", function(payload)
	if payload.user == ent:uid() then return end

	ent:callHook("io:FireOutput.%UID%", { output = "OnPlayerUse" })
	ent:callHook("io:FireOutput.%UID%", { output = "OnUse" })
	ent:callHook("io:FireOutput.%UID%", { output = "OnPressed" })
end)