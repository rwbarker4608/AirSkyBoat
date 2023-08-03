-----------------------------------
-- Zone: East_Sarutabaruta (116)
-----------------------------------
local ID = require('scripts/zones/East_Sarutabaruta/IDs')
require('scripts/quests/i_can_hear_a_rainbow')
require('scripts/globals/chocobo_digging')
require('scripts/globals/conquest')
require('scripts/globals/keyitems')
require('scripts/globals/missions')
require('scripts/globals/zone')
require('scripts/globals/events/harvest_festivals')
require('scripts/globals/events/starlight_celebrations')
require('scripts/globals/events/sunbreeze_festival')
-----------------------------------
local zoneObject = {}

zoneObject.onChocoboDig = function(player, precheck)
    return xi.chocoboDig.start(player, precheck)
end

zoneObject.onInitialize = function(zone)
    if xi.settings.main.ENABLE_WOTG == 1 then
        xi.mob.nmTODPersistCache(zone, ID.mob.DUKE_DECAPOD)
    end

    xi.events.starlightCelebration.applyStarlightDecorations(zone:getID())
    xi.events.sunbreeze_festival.showNPCs(zone:getID())
end

zoneObject.onZoneIn = function(player, prevZone)
    local cs = -1

    if
        player:getXPos() == 0 and
        player:getYPos() == 0 and
        player:getZPos() == 0
    then
        player:setPos(-125, -3, -519, 4)
    end

    if quests.rainbow.onZoneIn(player) then
        cs = 50
    elseif
        player:getCurrentMission(xi.mission.log_id.ASA) == xi.mission.id.asa.BURGEONING_DREAD and
        prevZone == xi.zone.WINDURST_WOODS and
        not player:hasStatusEffect(xi.effect.MOUNTED)
    then
        cs = 71
    end

    return cs
end

zoneObject.onConquestUpdate = function(zone, updatetype, influence, owner, ranking, isConquestAlliance)
    xi.conq.onConquestUpdate(zone, updatetype, influence, owner, ranking, isConquestAlliance)
end

zoneObject.onZoneTick = function(zone)
    xi.events.sunbreeze_festival.onZoneTick(zone)
end

zoneObject.onGameHour = function(zone)
    xi.events.sunbreeze_festival.spawnFireworks(zone)
end

zoneObject.onGameDay = function()
    SetServerVariable("[DIG]ZONE116_ITEMS", 0)
    if xi.events.starlightCelebration.isStarlightEnabled ~= 0 then
        xi.events.starlightCelebration.resetSmileHelpers(xi.zone.EAST_SARUTABARUTA)
    end
end

zoneObject.onTriggerAreaEnter = function(player, triggerArea)
end

zoneObject.onEventUpdate = function(player, csid, option)
    if csid == 50 then
        quests.rainbow.onEventUpdate(player)
    elseif csid == 71 then
        player:setCharVar("ASA_Status", option)
    end
end

zoneObject.onEventFinish = function(player, csid, option)
    if csid == 71 then
        player:completeMission(xi.mission.log_id.ASA, xi.mission.id.asa.BURGEONING_DREAD)
        player:addMission(xi.mission.log_id.ASA, xi.mission.id.asa.THAT_WHICH_CURDLES_BLOOD)
    end
end

return zoneObject
