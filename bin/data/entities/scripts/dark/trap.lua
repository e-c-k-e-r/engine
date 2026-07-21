local ent = ent
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local soundMeta = darkMeta["sound"] or {}
local connections = darkMeta["connections"] or {}

local isPlaying = false
local nativeSchema = soundMeta["schema"] or ""

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message

	if msg == "TurnOn" and not isPlaying then
		local explicitSchema = ""

		for _, conn in ipairs(connections) do
			if conn.flavor == "SoundDescription" and conn.target_node and conn.target_node ~= "UnknownSchema" then
				explicitSchema = conn.target_node
				break
			end
		end

		if explicitSchema == "" and nativeSchema ~= "" then
			explicitSchema = nativeSchema
		end

		if explicitSchema ~= "" then
			local resolvedUrl = _G.DarkUtils.playSound(ent, "", explicitSchema, {
				spatial = false, streamed = true, volume = 0.2, unique = true, loop = false
			})
			if resolvedUrl then isPlaying = true end
		end

	elseif msg == "TurnOff" and isPlaying then
		isPlaying = false
		ent:callHook("sound:Stop.%UID%", {})
	end
end)