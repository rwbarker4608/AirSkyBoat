-----------------------------------
-- Sleepga
-----------------------------------
require("scripts/globals/mobskills")
require("scripts/globals/magic")
require("scripts/globals/msg")
require("scripts/globals/spell_data")
require("scripts/globals/summon")
-----------------------------------
local abilityObject = {}

abilityObject.onAbilityCheck = function(player, target, ability)
    xi.job_utils.summoner.canUseBloodPact(player, player:getPet(), target, ability)
end

abilityObject.onPetAbility = function(target, pet, skill)
    local duration = 90
    local dINT = pet:getStat(xi.mod.INT) - target:getStat(xi.mod.INT)
    local bonus = xi.summon.getSummoningSkillOverCap(pet)
    local resm = xi.mobskills.applyPlayerResistance(pet, -1, target, dINT, bonus, xi.magic.ele.DARK)
    if resm < 0.5 then
        skill:setMsg(xi.msg.basic.JA_MISS_2) -- resist message
        return xi.effect.SLEEP_I
    end

    duration = duration * resm
    if target:hasImmunity(xi.immunity.SLEEP) or hasSleepEffects(target) then
        --No effect
        skill:setMsg(xi.msg.basic.SKILL_NO_EFFECT)
    else
        skill:setMsg(xi.msg.basic.SKILL_ENFEEB)

        target:addStatusEffect(xi.effect.SLEEP_I, 1, 0, duration)
    end

    return xi.effect.SLEEP_I
end

return abilityObject
