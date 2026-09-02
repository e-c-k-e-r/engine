local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local cases = {}
for i = 1, 20 do
	local value = metadataValve["Case" .. i]
	if value ~= nil and tostring(value) ~= "" then
		cases[i] = tostring(value)
	end
end

ent:addHook("io:Input.%UID%", function( payload )
	local param = payload.parameter
	local p = (param == nil) and "" or tostring(param)

	for i = 1, 20 do
		if cases[i] then
			if cases[i] == p or tonumber(cases[i]) == tonumber(p) then
				ent:callHook("io:FireOutput.%UID%", { output = "Case" .. i })
				break
			end
		end
	end
end)
