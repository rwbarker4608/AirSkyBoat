-----------------------------------
-- Crescent Fang
-- Fenrir inflicts Paralysis along with a single attack to target.
-----------------------------------
require("scripts/globals/mobskills")
-----------------------------------
local mobskillObject = {}

mobskillObject.onMobSkillCheck = function(target, mob, skill)
    return 0
end

mobskillObject.onMobWeaponSkill = function(target, mob, skill)
    local numhits = 1
    local accmod = 2
    local dmgmod = 1

    local totaldamage = 0
    local damage = xi.mobskills.mobPhysicalMove(mob, target, skill, numhits, accmod, dmgmod, 0, xi.mobskills.magicalTpBonus.NO_EFFECT, 1, 2, 3)
    totaldamage = xi.mobskills.mobFinalAdjustments(damage.dmg, mob, skill, target, xi.attackType.PHYSICAL, xi.damageType.PIERCING, numhits)

    if not skill:hasMissMsg() then
        target:addStatusEffect(xi.effect.PARALYSIS, 50, 0, 90)
        target:takeDamage(totaldamage, mob, xi.attackType.PHYSICAL, xi.damageType.PIERCING)
    end

    return totaldamage
end

return mobskillObject
