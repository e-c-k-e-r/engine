local ent = ent
local metadata = ent:getComponent("Metadata")
local darkMeta = metadata["dark"] or {}
local soundMeta = darkMeta["sound"] or {}

local schemaName = soundMeta["schema"] or ""
local flags = soundMeta["flags"] or 0
local baseVolume = tonumber(soundMeta["volume"]) or 1.0
local radius = tonumber(soundMeta["radius"]) or 0.0

local environmental = (math.floor(flags / 1) % 2) ~= 0
local isTurnedOff = (math.floor(flags / 4) % 2) ~= 0
local isMusic = (math.floor(flags / 16) % 2) ~= 0

if isMusic then return end
if schemaName == "" then return end

local isTurnedOff = (math.floor(flags / 4) % 2) ~= 0
local startOn = not isTurnedOff

local isPlaying = false

local function playSound()
	if isPlaying or schemaName == "" then return end
	local wavList = soundMeta["wavs"]
	local url = ""

	if wavList and #wavList > 0 then
		local totalWeight = 0
		for _, wavData in ipairs(wavList) do
			totalWeight = totalWeight + (tonumber(wavData.weight) or 1)
		end

		local roll = math.random() * totalWeight
		local current = 0
		for _, wavData in ipairs(wavList) do
			current = current + (tonumber(wavData.weight) or 1)
			if roll <= current then
				url = wavData.uri
				break
			end
		end
	end
	if url == "" then return end

	isPlaying = true
	local resolvedUrl = string.resolveURI(url, metadata["system"]["root"])

	local schemaVol = tonumber(soundMeta["schema_volume"]) or 0
	local overrideVol = tonumber(soundMeta["volume"]) or 0

	local baseVolume = 0.2
	if overrideVol ~= 0 then baseVolume = math.pow(10, overrideVol / 2000.0) end
	local finalVolume = baseVolume * math.pow(10, schemaVol / 2000.0)

	local payload = {
		filename = resolvedUrl,
		spatial = not environmental,
		streamed = soundMeta["stream"] == true,
		volume = finalVolume,
		unique = true,
		loop = not (soundMeta["play_once"] == true)
	}

	if radius > 0 and not environmental then
		payload.maxDistance = radius
		payload.referenceDistance = 1.0
		payload.rolloffFactor = 1.0
	end

	ent:callHook("sound:Emit.%UID%", payload)
end

local function stopSound()
	if not isPlaying or schemaName == "" then return end
	isPlaying = false

	ent:callHook("sound:Stop.%UID%", {
	})
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