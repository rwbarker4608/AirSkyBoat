-----------------------------------
-- Area: Port Windurst
--  NPC: Yaman-Hachuman
-- Type: Standard NPC
--  Involved in Quests: Wonder Wands
-- !pos -101.209 -4.25 110.886 240
-----------------------------------
require("scripts/globals/settings")
require("scripts/globals/quests")
require("scripts/globals/utils")
-----------------------------------
local entity = {}

entity.onTrade = function(player, npc, trade)
end

entity.onTrigger = function(player, npc)
    local wildcatWindurst = player:getCharVar("WildcatWindurst")

    if
        player:getQuestStatus(xi.quest.log_id.WINDURST, xi.quest.id.windurst.LURE_OF_THE_WILDCAT) == QUEST_ACCEPTED and
        not utils.mask.getBit(wildcatWindurst, 16)
    then
        player:startEvent(624)
    else
        player:startEvent(233)
    end
end

entity.onEventUpdate = function(player, csid, option)
end

entity.onEventFinish = function(player, csid, option)
    if csid == 624 then
        player:setCharVar("WildcatWindurst", utils.mask.setBit(player:getCharVar("WildcatWindurst"), 16, true))
    end
end

return entity
