-----------------------------------
-- ID: 5214
-- Item: dish_of_spaghetti_melanzane_+1
-- Food Effect: 1Hr, All Races
-----------------------------------
-- HP % 25 (cap 105)
-- Vitality 2
-- Store TP 6
-- Resist sleep 10
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
    target:addStatusEffect(xi.effect.FOOD, 0, 0, 3600, 5214)
end

itemObject.onEffectGain = function(target, effect)
    target:addMod(xi.mod.FOOD_HPP, 25)
    target:addMod(xi.mod.FOOD_HP_CAP, 105)
    target:addMod(xi.mod.VIT, 2)
    target:addMod(xi.mod.STORETP, 6)
    target:addMod(xi.mod.SLEEPRES, 10)
end

itemObject.onEffectLose = function(target, effect)
    target:delMod(xi.mod.FOOD_HPP, 25)
    target:delMod(xi.mod.FOOD_HP_CAP, 105)
    target:delMod(xi.mod.VIT, 2)
    target:delMod(xi.mod.STORETP, 6)
    target:delMod(xi.mod.SLEEPRES, 10)
end

return itemObject
