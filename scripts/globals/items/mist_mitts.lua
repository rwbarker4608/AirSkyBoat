-----------------------------------
-- ID: 14855
-- Item: Mist Mitts
-- Item Effect: Evasion Boost
-----------------------------------
require("scripts/globals/msg")
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    local effect = target:getStatusEffect(xi.effect.EVASION_BOOST)
    if effect ~= nil and effect:getItemSourceID() == xi.items.MIST_MITTS then
        target:delStatusEffect(xi.effect.EVASION_BOOST)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.MIST_MITTS) then
        if not target:hasStatusEffect(xi.effect.EVASION_BOOST) then
            target:addStatusEffect(xi.effect.EVASION_BOOST, 15, 0, 180, 0, 0, 0, xi.items.MIST_MITTS)
        else
            target:messageBasic(xi.msg.basic.NO_EFFECT)
        end
    end
end

return itemObject
