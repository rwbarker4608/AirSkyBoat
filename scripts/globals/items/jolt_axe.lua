-----------------------------------
-- ID: 17954
-- Item: jolt_axe
-- Item Effect: Attack +3
-- Duration: 30 Minutes
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    local effect = target:getStatusEffect(xi.effect.ATTACK_BOOST)
    if effect ~= nil and effect:getItemSourceID() == xi.items.JOLT_AXE then
        target:delStatusEffect(xi.effect.ATTACK_BOOST)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.JOLT_AXE) then
        target:addStatusEffect(xi.effect.ATTACK_BOOST, 3, 0, 1800, 0, 0, 0, xi.items.JOLT_AXE)
    end
end

return itemObject
