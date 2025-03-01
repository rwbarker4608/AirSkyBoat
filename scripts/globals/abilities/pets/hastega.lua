-----------------------------------
-- Hastega
-----------------------------------
require("scripts/globals/mobskills")
require("scripts/globals/utils")
require("scripts/globals/msg")
-----------------------------------
local abilityObject = {}

abilityObject.onAbilityCheck = function(player, target, ability)
    return 0, 0
end

abilityObject.onPetAbility = function(target, pet, skill, summoner)
    local duration = math.min(90 + xi.summon.getSummoningSkillOverCap(pet) * 3, 180)
    -- Garuda's Hastega is a weird exception and uses 153/1024 instead of 150/1024 like Haste spell
    -- That's why it overwrites some things regular haste won't.
    target:addStatusEffect(xi.effect.HASTE, 1494, 0, duration) -- 153/1024 ~14.94%
    skill:setMsg(xi.msg.basic.SKILL_GAIN_EFFECT)
    return xi.effect.HASTE
end

return abilityObject
