-----------------------------------
-- ID: 14517
-- Item: Hydra Haubert
-- Item Effect: gives refresh
-----------------------------------
require("scripts/globals/msg")
-----------------------------------
local itemObject = {}

itemObject.onItemCheck = function(target)
    local effect = target:getStatusEffect(xi.effect.REFRESH)
    if
        effect ~= nil and
        effect:getItemSourceID() == xi.items.HYDRA_HAUBERT
    then
        target:delStatusEffect(xi.effect.REFRESH)
    end

    return 0
end

itemObject.onItemUse = function(target)
    if target:hasEquipped(xi.items.HYDRA_HARNESS) then
        if target:hasStatusEffect(xi.effect.REFRESH) then
            target:messageBasic(xi.msg.basic.NO_EFFECT)
        else
            target:addStatusEffect(xi.effect.REFRESH, 4, 3, 180, 0, 0, 0, xi.items.HYDRA_HAUBERT)
        end
    end
end

return itemObject
