-----------------------------------
-- Aerial Blast
-----------------------------------
require("scripts/globals/settings")
require("scripts/globals/magic")
require("scripts/globals/mobskills")
-----------------------------------
local abilityObject = {}

abilityObject.onAbilityCheck = function(player, target, ability)
    local level = player:getMainLvl() * 2

    if player:getMP() < level then
        return 87, 0
    end

    xi.job_utils.summoner.canUseBloodPact(player, player:getPet(), target, ability)
end

abilityObject.onPetAbility = function(target, pet, skill, summoner)
    local params = {}
    params.ftp000 = 9 params.ftp150 = 9 params.ftp300 = 9
    params.str_wsc = 0.0 params.dex_wsc = 0.0 params.vit_wsc = 0.0 params.agi_wsc = 0.0 params.int_wsc = 0.3 params.mnd_wsc = 0.0 params.chr_wsc = 0.0
    params.element = xi.magic.ele.WIND
    params.includemab = true
    params.maccBonus = xi.summon.getSummoningSkillOverCap(pet)
    params.damageSpell = true

    local damage = xi.summon.avatarMagicSkill(pet, target, skill, params)

    summoner:setMP(0)
    local totaldamage = xi.summon.avatarFinalAdjustments(damage.dmg, pet, skill, target, xi.attackType.MAGICAL, xi.damageType.WIND, xi.mobskills.shadowBehavior.WIPE_SHADOWS)
    target:takeDamage(totaldamage, pet, xi.attackType.MAGICAL, xi.damageType.WIND)

    xi.magic.handleSMNBurstMsg(pet, target, skill, params.element, 379)

    return totaldamage
end

return abilityObject
