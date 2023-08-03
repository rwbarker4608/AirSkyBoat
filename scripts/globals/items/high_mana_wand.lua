-----------------------------------
-- ID: 18403
-- Item: high_mana_wand
-- Item Effect: MPHEAL +4
-- Duration: 30 seconds
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    if target:getStatusEffect(xi.effect.ENCHANTMENT, nil, xi.items.HIGH_MANA_WAND) ~= nil then
        target:delStatusEffect(xi.effect.ENCHANTMENT, nil, xi.items.HIGH_MANA_WAND)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.HIGH_MANA_WAND) then
        target:addStatusEffect(xi.effect.ENCHANTMENT, 0, 0, 1800, 0, 0, 0, xi.items.HIGH_MANA_WAND)
    end
end

itemObject.onEffectGain = function(target, effect)
    target:addMod(xi.mod.MPHEAL, 4)
end

itemObject.onEffectLose = function(target, effect)
    target:delMod(xi.mod.MPHEAL, 4)
end

return itemObject
