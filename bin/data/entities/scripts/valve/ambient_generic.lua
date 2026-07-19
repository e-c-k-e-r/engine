local ent = ent
local metadata = ent:getComponent("Metadata")
local metadataValve = metadata["valve"] or {}

local soundFile = metadataValve["message"] or ""
local flags = metadataValve["spawnflags"] or 0

local volume = tonumber(metadataValve["health"]) or 10.0
volume = volume / 10.0

local playEverywhere = (math.floor(flags / 1) % 2) ~= 0
local startSilent	= (math.floor(flags / 16) % 2) ~= 0
local isNotLooped	= (math.floor(flags / 32) % 2) ~= 0

local isPlaying = false

local function playSound()
	if isPlaying or soundFile == "" then return end
	isPlaying = true

	local url = "valve://sound/" .. soundFile

	local payload = {
		filename = string.resolveURI(url, metadata["system"]["root"]),
		spatial = not playEverywhere,
		streamed = true,
		volume = volume,
		unique = true
	}
	payload["wants loop"] = not isNotLooped
	ent:callHook("sound:Emit.%UID%", payload)
end

local function stopSound()
	if not isPlaying or soundFile == "" then return end
	isPlaying = false

	local url = "valve://sound/" .. soundFile
	ent:callHook("sound:Stop.%UID%", {
		filename = string.resolveURI(url, metadata["system"]["root"])
	})
end

ent:addHook("io:Input.%UID%", function(payload)
	local input = payload.input

	if input == "PlaySound" then
		playSound()
	elseif input == "StopSound" then
		stopSound()
	elseif input == "ToggleSound" then
		if isPlaying then
			stopSound()
		else
			playSound()
		end
	elseif input == "Volume" then
		local newVol = tonumber(payload.parameter)
		if newVol then
			volume = newVol / 10.0
			-- to-do: update volume of currently playing sound
		end
	end
end)

if not startSilent then
	playSound()
end