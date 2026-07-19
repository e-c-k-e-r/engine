local scene = entities.currentScene()
local metadata = ent:getComponent("Metadata")
local activeSongId = metadata["dark"]["mission song"] or "song02"
local songs = metadata["dark"]["songs"] or {}
local songData = songs[activeSongId]

local bakedMarkers = metadata["dark"]["music markers"] or {}
local musicMarkers = {}
local activeMarkers = {}

local currentSectionIdx = 0
local currentLoopCount = 0
local initializedPlayer = false

if not songData then
	print("[Music Manager] ERROR: Active song '" .. tostring(activeSongId) .. "' not found!")
	return
end

print("[Music Manager] Bound Mission Song: " .. activeSongId)

for _, markerData in ipairs(bakedMarkers) do
	table.insert(musicMarkers, {
		position = Vector3f(markerData.position[1], markerData.position[2], markerData.position[3]),
		radius = tonumber(markerData.radius) or 0,
		theme = string.lower(markerData.theme or "")
	})
end

print("[Music Manager] Pre-caching song assets...")
for _, section in ipairs(songData.sections or {}) do
	if section.samples then
		for _, sampleName in ipairs(section.samples) do
			local url = "SND://songs/" .. string.lower(sampleName)
			if not string.match(url, "%.wav$") then url = url .. ".wav" end
			ent:callHook("asset:QueueLoad.%UID%", { uri = url })
		end
	end
end

local function getSampleUrl(section)
	if section.samples and #section.samples > 0 then
		local url = "SND://songs/" .. string.lower(section.samples[1])
		if not string.match(url, "%.wav$") then url = url .. ".wav" end
		return url
	end
	return nil
end

local function predictNextSection(sectionIdx, loopCount)
	local section = songData.sections[sectionIdx + 1]
	local maxLoops = section.loopCount or 0

	if maxLoops > 0 and loopCount < maxLoops then
		return sectionIdx, loopCount + 1
	end

	local eventFound = nil
	if section and section.events then
		for _, evt in ipairs(section.events) do
			if evt.event == "" then eventFound = evt; break end
		end
	end
	if not eventFound and songData.events then
		for _, evt in ipairs(songData.events) do
			if evt.event == "" then eventFound = evt; break end
		end
	end

	if eventFound and eventFound.gotos and #eventFound.gotos > 0 then
		return eventFound.gotos[1].section, 0
	end

	return nil, 0
end

local function playSection(sectionIdx, isQueue)
	if not songData.sections or not songData.sections[sectionIdx + 1] then return end

	local section = songData.sections[sectionIdx + 1]
	currentSectionIdx = sectionIdx

	local url = getSampleUrl(section)
	if url then
		if isQueue then
			print(string.format("[Music Manager] Queuing section: %s", section.id))
			ent:callHook("sound:QueueTrack.%UID%", { filename = url, layer = 1 })
		else
			print(string.format("[Music Manager] Forcing immediate section: %s", section.id))
			ent:callHook("sound:PlayTrack.%UID%", { filename = url, layer = 1, loop = false })

			-- Immediately predict and queue the next track to prevent gaps!
			local nextIdx, nextLoop = predictNextSection(currentSectionIdx, currentLoopCount)
			if nextIdx then
				local nextSec = songData.sections[nextIdx + 1]
				local nextUrl = getSampleUrl(nextSec)
				if nextUrl then
					print(string.format("[Music Manager] Pre-queuing next section: %s", nextSec.id))
					ent:callHook("sound:QueueTrack.%UID%", { filename = nextUrl, layer = 1 })
				end
			end
		end
	else
		processEvent("")
	end
end

local function processEvent(eventName)
	local altEventName = "theme " .. eventName
	local section = songData.sections[currentSectionIdx + 1]
	local eventFound = nil

	if section and section.events then
		for _, evt in ipairs(section.events) do
			if evt.event == eventName or evt.event == altEventName then
				eventFound = evt
				break
			end
		end
	end

	if not eventFound and songData.events then
		for _, evt in ipairs(songData.events) do
			if evt.event == eventName or evt.event == altEventName then
				eventFound = evt
				break
			end
		end
	end

	if eventFound and eventFound.gotos and #eventFound.gotos > 0 then
		local randNum = math.random(0, 99)
		local totalProbability = 0
		local targetSection = eventFound.gotos[1].section

		for _, gotoData in ipairs(eventFound.gotos) do
			totalProbability = totalProbability + (gotoData.probability or 100)
			if randNum < totalProbability then
				targetSection = gotoData.section
				break
			end
		end

		print(string.format("[Music Manager] Event '%s' triggered transition to section %d", eventFound.event, targetSection))

		if eventName ~= "" then
			currentLoopCount = 0
			playSection(targetSection, false)
		else
			playSection(targetSection, true)
		end
	else
		print(string.format("[Music Manager] WARNING: Event '%s' ignored (not valid from section %d)", eventName, currentSectionIdx))
	end
end

ent:addHook( "sound:TrackEnded.%UID%", function(self, payload)
	local section = songData.sections[currentSectionIdx + 1]
	local maxLoops = section.loopCount or 0

	if maxLoops == 0 or currentLoopCount >= maxLoops then
		currentLoopCount = 0
		processEvent("")
	else
		currentLoopCount = currentLoopCount + 1
		playSection(currentSectionIdx, true)
	end
end)

local function updateMusicState(playerPos, forceInitialization)
	for id, marker in ipairs(musicMarkers) do
		local dist = playerPos:distance(marker.position)
		local isInside = (dist <= marker.radius)

		if isInside and (not activeMarkers[id] or forceInitialization) then
			activeMarkers[id] = true
			processEvent(marker.theme)
		elseif not isInside and activeMarkers[id] then
			activeMarkers[id] = nil
		end
	end
end

ent:bind( "tick", function(self)
	local player = scene:findByName("Player")
	if not player then return end

	local playerPos = player:getComponent("Transform").position

	if not initializedPlayer then
		initializedPlayer = true
		processEvent("begin")
		updateMusicState(playerPos, true)
	else
		updateMusicState(playerPos, false)
	end
end)