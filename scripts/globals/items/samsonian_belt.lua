-----------------------------------
-- ID: 15863
-- Item: samsonian_belt
-- Item Effect: STR +3
-- Duration: 60 seconds
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    local effect = target:getStatusEffect(xi.effect.STR_BOOST)
    if effect ~= nil and effect:getItemSourceID() == xi.items.SAMSONIAN_BELT then
        target:delStatusEffect(xi.effect.STR_BOOST)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.SAMSONIAN_BELT) then
        target:addStatusEffect(xi.effect.STR_BOOST, 3, 0, 60, 0, 0, 0, xi.items.SAMSONIAN_BELT)
    end
end

return itemObject
