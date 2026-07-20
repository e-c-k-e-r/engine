local ent = ent
local metadata = ent:getComponent("Metadata")
local db = metadata["dark"]["schema_db"] or {}

local function parseTags(tagString)
	local result = {}
	if not tagString or tagString == "" then return result end

	for pair in string.gmatch(tagString, "([^,]+)") do
		pair = string.match(pair, "^%s*(.-)%s*$")
		local key, val = string.match(pair, "^(%S+)%s+(.+)$")
		if key and val then
			result[key] = val
		else
			result[pair] = true
		end
	end
	return result
end

local function matchValue(sVal, qVal)
	if sVal == qVal then return true end

	if type(sVal) == "string" and type(qVal) == "string" then
		for piece in string.gmatch(sVal, "([^|]+)") do
			if piece == qVal then return true end
		end
	end

	if sVal == true and qVal ~= nil then return true end

	return false
end

hooks.add("dark:ResolveSchema", function(payload)
	local explicitSchema = payload.schema or ""

	if explicitSchema ~= "" then
		for _, schema in ipairs(db) do
			if schema.name:lower() == explicitSchema:lower() then
				return schema
			end
		end
	end

	local queryTags = parseTags(payload.tags)
	local bestMatch = nil
	local bestScore = -1

	for _, schema in ipairs(db) do
		local sTags = parseTags(schema.tags)
		local score = 0
		local isValid = true

		if queryTags["Event"] and not sTags["Event"] then
			isValid = false
		end

		if isValid then
			for sKey, sVal in pairs(sTags) do
				local qVal = queryTags[sKey]

				if qVal == nil then
					isValid = false
					break
				elseif matchValue(sVal, qVal) then
					score = score + 10
				elseif matchValue(sVal, "Gen") or matchValue(sVal, "Generic") then
					score = score + 5
				else
					isValid = false
					break
				end
			end
		end

		if isValid and score > bestScore then
			if next(sTags) ~= nil then
				bestScore = score
				bestMatch = schema
			end
		end
	end

	if bestMatch then
		bestMatch.score = bestScore
		return bestMatch
	end

	return nil
end)