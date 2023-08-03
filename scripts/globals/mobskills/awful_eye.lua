-----------------------------------
-- Awful Eye
-- 15' Reduces STR of players in area of effect.
-----------------------------------
require("scripts/globals/mobskills")
-----------------------------------
local mobskillObject = {}

mobskillObject.onMobSkillCheck = function(target, mob, skill)
    return 0
end

mobskillObject.onMobWeaponSkill = function(target, mob, skill)
    skill:setMsg(xi.mobskills.mobGazeMove(mob, target, xi.effect.STR_DOWN, 10, 10, 180))

    return xi.effect.STR_DOWN
end

return mobskillObject
