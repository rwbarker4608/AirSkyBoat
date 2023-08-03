-----------------------------------
--
-----------------------------------
require("scripts/globals/mobskills")
require("scripts/globals/utils")
-----------------------------------
local abilityObject = {}

abilityObject.onAbilityCheck = function(player, target, ability)
    xi.job_utils.summoner.canUseBloodPact(player, player:getPet(), target, ability)
end

abilityObject.onPetAbility = function(target, pet, skill, summoner)
    local duration = math.min(60 + xi.summon.getSummoningSkillOverCap(pet) * 2, 180)
    local magicskill = utils.getSkillLvl(1, target:getMainLvl())
    local potency = 3 + ((6 * magicskill) / 100)
    if magicskill > 200 then
        potency = 5 + ((5 * magicskill) / 100)
    end

    local typeEffect = xi.effect.ENTHUNDER

    skill:setMsg(xi.mobskills.mobBuffMove(target, typeEffect, potency, 0, duration))

    return typeEffect
end

return abilityObject
