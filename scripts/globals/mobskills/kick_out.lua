-----------------------------------
-- Kick Out
--
-- Description: Deals heavy damage and inflicts blind to any target behind user.
-- Type: Physical
-- Utsusemi/Blink absorb: 2-3 shadows
-- Range: Unknown cone, backwards
-- Notes:  Only used when the Behemoth is attacking with its tail.
-----------------------------------
require("scripts/globals/mobskills")
require("scripts/globals/settings")
require("scripts/globals/status")
-----------------------------------
local mobskillObject = {}

mobskillObject.onMobSkillCheck = function(target, mob, skill)
    if not target:isBehind(mob, 48) then
        return 1
    end

    return 0
end

mobskillObject.onMobWeaponSkill = function(target, mob, skill)
    local numhits = math.random(2, 3)
    local accmod = 1
    local dmgmod = 1
    local info = xi.mobskills.mobPhysicalMove(mob, target, skill, numhits, accmod, dmgmod, xi.mobskills.magicalTpBonus.NO_EFFECT)
    local dmg = xi.mobskills.mobFinalAdjustments(info.dmg, mob, skill, target, xi.attackType.PHYSICAL, xi.damageType.HTH, info.hitslanded)

    local typeEffect = xi.effect.BLINDNESS

    xi.mobskills.mobPhysicalStatusEffectMove(mob, target, skill, typeEffect, 20, 0, 120)

    if not skill:hasMissMsg() then
        target:takeDamage(dmg, mob, xi.attackType.PHYSICAL, xi.damageType.HTH)
    end

    return dmg
end

return mobskillObject
