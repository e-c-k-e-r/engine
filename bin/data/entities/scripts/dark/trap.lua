local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local connections = darkMeta["connections"] or {}

ent:addHook("dark:Message.%UID%", function(payload)
	local msg = payload.message

	if msg == "TurnOn" then
		for _, conn in ipairs(connections) do
			if conn.flavor == "SoundDescription" and conn.wavs and #conn.wavs > 0 then
				local pick = conn.wavs[math.random(#conn.wavs)]
				local resolvedUrl = string.resolveURI(pick, metadata["system"]["root"])

				ent:callHook("sound:Emit.%UID%", {
					filename = resolvedUrl,
					spatial = true,
					streamed = false,
					volume = 1.0,
					unique = true,
					loop = false
				})
				break
			end
		end
	end
end)