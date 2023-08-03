-----------------------------------
-- ID: 18586
-- Item: flexible_pole
-- Item Effect: Attack +3
-- Duration: 30 Minutes
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    if target:getStatusEffect(xi.effect.ENCHANTMENT, nil, xi.items.FLEXIBLE_POLE) ~= nil then
        target:delStatusEffect(xi.effect.ENCHANTMENT, nil, xi.items.FLEXIBLE_POLE)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.FLEXIBLE_POLE) then
        target:addStatusEffect(xi.effect.ENCHANTMENT, 0, 0, 1800, 0, 0, 0, xi.items.FLEXIBLE_POLE)
    end
end

itemObject.onEffectGain = function(target, effect)
    target:addMod(xi.mod.ATT, 3)
end

itemObject.onEffectLose = function(target, effect)
    target:delMod(xi.mod.ATT, 3)
end

return itemObject
