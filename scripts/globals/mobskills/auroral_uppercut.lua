-----------------------------------
--  Auroral Uppercut
-----------------------------------
local ID = require("scripts/zones/Empyreal_Paradox/IDs")
require("scripts/globals/mobskills")
-----------------------------------
local mobskillObject = {}

mobskillObject.onMobSkillCheck = function(target, mob, skill)
    if
        target:hasStatusEffect(xi.effect.PHYSICAL_SHIELD) or
        target:hasStatusEffect(xi.effect.MAGIC_SHIELD)
    then
        return 1
    end

    return 0
end

mobskillObject.onMobWeaponSkill = function(target, mob, skill)
    mob:showText(mob, ID.text.PRISHE_TEXT + 4)
    local numhits = 1
    local accmod  = 1
    local dmgmod  = 1
    local info    = xi.mobskills.mobPhysicalMove(mob, target, skill, numhits, accmod, dmgmod, xi.mobskills.physicalTpBonus.DMG_VARIES, 1, 2, 3)
    local dmg     = xi.mobskills.mobFinalAdjustments(info.dmg, mob, skill, target, xi.attackType.PHYSICAL, xi.damageType.BLUNT, info.hitslanded)

    if not skill:hasMissMsg() then
        target:takeDamage(dmg, mob, xi.attackType.PHYSICAL, xi.damageType.BLUNT)
    end

    return dmg
end

return mobskillObject
