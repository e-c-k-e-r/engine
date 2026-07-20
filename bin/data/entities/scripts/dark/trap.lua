local ent = ent
local scene = entities.currentScene()

local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local soundMeta = darkMeta["sound"] or {}
local schemaDb = darkMeta["schema_db"] or {}
local connections = darkMeta["connections"] or {}

local isPlaying = false

local nativeSchema = soundMeta["schema"] or ""

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message

	if msg == "TurnOn" and not isPlaying then
		local urlToPlay = ""

		for _, conn in ipairs(connections) do
			if conn.flavor == "SoundDescription" and conn.wavs and #conn.wavs > 0 then
				urlToPlay = conn.wavs[math.random(#conn.wavs)]
				break
			end
		end

		if urlToPlay == "" and nativeSchema ~= "" then
			for _, entry in ipairs(schemaDb) do
				if entry.name:lower() == nativeSchema:lower() then
					if entry.wavs and #entry.wavs > 0 then
						urlToPlay = entry.wavs[math.random(#entry.wavs)]
					end
					break
				end
			end
		end

		if urlToPlay ~= "" then
			local resolvedUrl = string.resolveURI(urlToPlay, metadata["system"]["root"])
			isPlaying = true

			ent:callHook("sound:Emit.%UID%", {
				filename = resolvedUrl,
				spatial = false,
				streamed = true,
				volume = 1.0,
				unique = true,
				loop = false
			})
		end

	elseif msg == "TurnOff" and isPlaying then
		isPlaying = false
		ent:callHook("sound:Stop.%UID%", {})
	end
end)