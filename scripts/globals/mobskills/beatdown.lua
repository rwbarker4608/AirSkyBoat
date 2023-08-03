-----------------------------------
-- Beatdown
-- Deals damage to a single target. Additional effect: bind
-----------------------------------
require("scripts/globals/mobskills")
-----------------------------------
local mobskillObject = {}

mobskillObject.onMobSkillCheck = function(target, mob, skill)
    return 0
end

mobskillObject.onMobWeaponSkill = function(target, mob, skill)
    local numhits = 1
    local attmod = 2
    local info = xi.mobskills.mobPhysicalMove(mob, target, skill, numhits, 1, 1, xi.mobskills.physicalTpBonus.CRIT_VARIES, 1, 1.5, 2, 0, attmod)
    local dmg = xi.mobskills.mobFinalAdjustments(info.dmg, mob, skill, target, xi.attackType.PHYSICAL, xi.damageType.BLUNT, info.hitslanded)
    if not skill:hasMissMsg() then
        target:takeDamage(dmg, mob, xi.attackType.PHYSICAL, xi.damageType.BLUNT)
    end

    xi.mobskills.mobPhysicalStatusEffectMove(mob, target, skill, xi.effect.BIND, 1, 0, 60)

    return dmg
end

return mobskillObject
