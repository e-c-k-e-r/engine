local ent = ent
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local soundMeta = darkMeta["sound"] or {}

local schemaName = soundMeta["schema"] or ""
local flags = soundMeta["flags"] or 0
local overrideVol = tonumber(soundMeta["volume"]) or 0
local radius = tonumber(soundMeta["radius"]) or 0.0

local environmental = (math.floor(flags / 1) % 2) ~= 0
local isTurnedOff = (math.floor(flags / 4) % 2) ~= 0
local isMusic = (math.floor(flags / 16) % 2) ~= 0

if isMusic or schemaName == "" then return end

local startOn = not isTurnedOff
local isPlaying = false

local function playSound()
	if isPlaying then return end

	local schemaData = hooks.call("dark:ResolveSchema", { schema = schemaName })
	if not schemaData or not schemaData.wavs or #schemaData.wavs == 0 then return end

	local pick = schemaData.wavs[math.random(#schemaData.wavs)]
	local resolvedUrl = string.resolveURI(pick, metadata["system"]["root"])

	local schemaVol = tonumber(schemaData.schema_volume) or 0
	local baseVolume = 0.2
	if overrideVol ~= 0 then baseVolume = math.pow(10, overrideVol / 2000.0) end
	local finalVolume = baseVolume * math.pow(10, schemaVol / 2000.0)

	local payload = {
		filename = resolvedUrl,
		spatial = not environmental,
		streamed = schemaData.stream == true,
		volume = finalVolume,
		unique = true,
		loop = not (schemaData.play_once == true)
	}

	if radius > 0 and not environmental then
		payload.maxDistance = radius
		payload.referenceDistance = 1.0
		payload.rolloffFactor = 1.0
	end

	isPlaying = true
	ent:callHook("sound:Emit.%UID%", payload)
end

local function stopSound()
	if not isPlaying then return end
	isPlaying = false
	ent:callHook("sound:Stop.%UID%", {})
end

ent:addHook("link:Message.%UID%", function(payload)
	local msg = payload.message
	if msg == "TurnOn" then
		playSound()
	elseif msg == "TurnOff" then
		stopSound()
	end
end)

if startOn then
	playSound()
end