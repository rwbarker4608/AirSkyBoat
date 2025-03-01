-----------------------------------
-- ID: 5917
-- Item: Choco-scroll
-- Food Effect: 3 Min, All Races
-----------------------------------
-- Mind 1
-- Speed 12.5%
-----------------------------------
require("scripts/globals/msg")
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    local result = 0
    if
        target:hasStatusEffect(xi.effect.FOOD) or
        target:hasStatusEffect(xi.effect.FIELD_SUPPORT_FOOD)
    then
        result = xi.msg.basic.IS_FULL
    end

    return result
end

itemObject.onItemUse = function(target)
    target:addStatusEffect(xi.effect.FOOD, 0, 0, 180, 5917)
end

itemObject.onEffectGain = function(target, effect)
    target:addMod(xi.mod.MND, 1)
    target:addMod(xi.mod.MOVE, 13)
end

itemObject.onEffectLose = function(target, effect)
    target:delMod(xi.mod.MND, 1)
    target:delMod(xi.mod.MOVE, 13)
end

return itemObject
