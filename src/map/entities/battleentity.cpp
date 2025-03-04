﻿/*
===========================================================================

  Copyright (c) 2010-2015 Darkstar Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

===========================================================================
*/

#include "common/logging.h"
#include "common/utils.h"

#include "battleentity.h"

#include "../ai/ai_container.h"
#include "../ai/states/attack_state.h"
#include "../ai/states/death_state.h"
#include "../ai/states/despawn_state.h"
#include "../ai/states/inactive_state.h"
#include "../ai/states/magic_state.h"
#include "../ai/states/raise_state.h"
#include "../ai/states/weaponskill_state.h"
#include "../attack.h"
#include "../attackround.h"
#include "../enmity_container.h"
#include "../items/item_weapon.h"
#include "../job_points.h"
#include "../lua/luautils.h"
#include "../mob_modifier.h"
#include "../notoriety_container.h"
#include "../packets/action.h"
#include "../recast_container.h"
#include "../roe.h"
#include "../status_effect_container.h"
#include "../utils/battleutils.h"
#include "../utils/mobutils.h"
#include "../utils/petutils.h"
#include "../utils/puppetutils.h"
#include "../weapon_skill.h"

CBattleEntity::CBattleEntity()
{
    TracyZoneScoped;
    m_OwnerID.clean();
    m_ModelRadius = 0;
    m_mlvl        = 0;
    m_slvl        = 0;

    m_mjob = JOB_WAR;
    m_sjob = JOB_WAR;

    m_magicEvasion = 0;

    m_Weapons[SLOT_MAIN]   = new CItemWeapon(0);
    m_Weapons[SLOT_SUB]    = new CItemWeapon(0);
    m_Weapons[SLOT_RANGED] = new CItemWeapon(0);
    m_Weapons[SLOT_AMMO]   = new CItemWeapon(0);
    m_dualWield            = false;

    memset(&health, 0, sizeof(health));
    health.maxhp = 1;

    PPet          = nullptr;
    PParty        = nullptr;
    PMaster       = nullptr;
    PLastAttacker = nullptr;

    StatusEffectContainer = std::make_unique<CStatusEffectContainer>(this);
    PRecastContainer      = std::make_unique<CRecastContainer>(this);
    PNotorietyContainer   = std::make_unique<CNotorietyContainer>(this);

    m_modStat[Mod::SLASH_SDT]  = 1000;
    m_modStat[Mod::PIERCE_SDT] = 1000;
    m_modStat[Mod::HTH_SDT]    = 1000;
    m_modStat[Mod::IMPACT_SDT] = 1000;
    m_modStat[Mod::MOVE]       = 0;

    m_Immunity   = 0;
    isCharmed    = false;
    m_unkillable = false;
    m_dmgType    = DAMAGE_TYPE::NONE;

    m_DeathType = DEATH_TYPE::NONE;

    BattleHistory.lastHitTaken_atkType = ATTACK_TYPE::NONE;
}

CBattleEntity::~CBattleEntity() = default;

bool CBattleEntity::isDead()
{
    return (health.hp <= 0 || status == STATUS_TYPE::DISAPPEAR || (PAI && PAI->IsCurrentState<CDeathState>()) || (PAI && PAI->IsCurrentState<CDespawnState>()));
}

bool CBattleEntity::isAlive()
{
    return !isDead();
}

bool CBattleEntity::isInDynamis()
{
    if (loc.zone != nullptr)
    {
        return loc.zone->GetType() == ZONE_TYPE::DYNAMIS;
    }
    return false;
}

bool CBattleEntity::isInAssault()
{
    if (loc.zone != nullptr)
    {
        return loc.zone->GetType() == ZONE_TYPE::DUNGEON_INSTANCED &&
               (loc.zone->GetRegionID() >= REGION_TYPE::WEST_AHT_URHGAN && loc.zone->GetRegionID() <= REGION_TYPE::ALZADAAL);
    }
    return false;
}

// return true if the mob has immunity
bool CBattleEntity::hasImmunity(uint32 imID)
{
    if (objtype == TYPE_MOB || objtype == TYPE_PET)
    {
        IMMUNITY mobImmunity = (IMMUNITY)imID;
        return (m_Immunity & mobImmunity);
    }
    return false;
}

bool CBattleEntity::isAsleep()
{
    return PAI->IsCurrentState<CInactiveState>();
}

bool CBattleEntity::isMounted()
{
    return (animation == ANIMATION_CHOCOBO || animation == ANIMATION_MOUNT);
}

bool CBattleEntity::isSitting()
{
    return (animation == ANIMATION_HEALING || animation == ANIMATION_SIT || (animation >= ANIMATION_SITCHAIR_0 && animation <= ANIMATION_SITCHAIR_10));
}

/************************************************************************
 *                                                                       *
 *  Пересчитываем максимальные значения hp и mp с учетом модификаторов   *
 *                                                                       *
 ************************************************************************/

void CBattleEntity::UpdateHealth()
{
    TracyZoneScoped;
    int32 dif = (getMod(Mod::CONVMPTOHP) - getMod(Mod::CONVHPTOMP));

    health.modmp = std::max(0, ((health.maxmp) * (100 + getMod(Mod::MPP)) / 100) +
                                   std::min<int16>((health.maxmp * m_modStat[Mod::FOOD_MPP] / 100), m_modStat[Mod::FOOD_MP_CAP]) + getMod(Mod::MP));
    health.modhp = std::max(1, ((health.maxhp) * (100 + getMod(Mod::HPP)) / 100) +
                                   std::min<int16>((health.maxhp * m_modStat[Mod::FOOD_HPP] / 100), m_modStat[Mod::FOOD_HP_CAP]) + getMod(Mod::HP));

    dif = (health.modmp - 0) < dif ? (health.modmp - 0) : dif;
    dif = (health.modhp - 1) < -dif ? -(health.modhp - 1) : dif;

    health.modhp += dif;
    health.modmp -= dif;

    if (objtype == TYPE_PC)
    {
        health.modhp = std::clamp(health.modhp, 1, 9999);
        health.modmp = std::clamp(health.modmp, 0, 9999);
    }

    health.hp = std::clamp(health.hp, 0, health.modhp);
    health.mp = std::clamp(health.mp, 0, health.modmp);

    updatemask |= UPDATE_HP;
}

/************************************************************************
 *                                                                       *
 *  Получаем текущее количество очков жизней                             *
 *                                                                       *
 ************************************************************************/

uint8 CBattleEntity::GetHPP() const
{
    uint8 hpp = (uint8)floor(((float)health.hp / (float)GetMaxHP()) * 100);
    // handle the edge case where a floor would show a mob with 1/1000 hp as 0
    if (hpp == 0 && health.hp > 0)
    {
        hpp = 1;
    }

    return hpp;
}

uint8 CBattleEntity::GetHPPNoPercentOrConvert()
{
    TracyZoneScoped;
    // recalculate max HP using only raw HP mods; no convert or HPP, regardless of source, apply
    int32 modHP = std::max(1, health.maxhp + getMod(Mod::HP));

    uint8 hpp = (uint8)floor(((float)health.hp / (float)modHP) * 100);

    // it may be possible that with convert gear, this hpp would be > 100, so clamp it
    // this also handles the case where floor() returns 0 with 1/1000 hp
    return health.hp <= 0 ? 0 : std::clamp<uint8>(hpp, 1, 100);
}

int32 CBattleEntity::GetMaxHP() const
{
    return health.modhp;
}

/************************************************************************
 *                                                                       *
 *  Получаем текущее количество очков маны                               *
 *                                                                       *
 ************************************************************************/

uint8 CBattleEntity::GetMPP() const
{
    return (uint8)ceil(((float)health.mp / (float)GetMaxMP()) * 100);
}

int32 CBattleEntity::GetMaxMP() const
{
    return health.modmp;
}

/************************************************************************
 *                                                                       *
 *  Movement speed, taking into account modifiers                        *
 *  Note: retail speeds show as a float in the client,                   *
 *        yet in the packet it seems to be just one byte 0-255..         *
 *                                                                       *
 ************************************************************************/

uint8 CBattleEntity::GetSpeed()
{
    // Note: retail treats mounted speed as double what it actually is! 40 is in fact retail accurate!
    int16 startingSpeed = isMounted() ? 40 + settings::get<int8>("map.MOUNT_SPEED_MOD") : speed;
    // Mod::MOVE (169)
    // Mod::MOUNT_MOVE (972)
    Mod mod = isMounted() ? Mod::MOUNT_MOVE : Mod::MOVE;

    // Get the percentage of increase/decreate by mod
    auto modAmount = static_cast<float>(getMod(mod)) / 100.0f;

    // Cap unmounted movement speed increase to 25%
    if (mod == Mod::MOVE && modAmount > 0) // Only clamp position/negatvie if we're increasing speed
    {
        if (StatusEffectContainer->GetStatusEffect(EFFECT_FLEE))
        {
            modAmount = std::clamp(modAmount, 0.0f, 2.0f);
        }
        else
        {
            modAmount = std::clamp(modAmount, 0.0f, 1.25f);
        }
    }

    // Add speed mod to base speed
    auto modifiedSpeed = startingSpeed + static_cast<float>(startingSpeed) * modAmount;

    // Make sure speed cannot go negative
    auto outputSpeed = static_cast<uint8>(modifiedSpeed < 0 ? 0 : modifiedSpeed);

    // Actually need this clamp?
    return std::clamp<uint8>(outputSpeed, std::numeric_limits<uint8>::min(), std::numeric_limits<uint8>::max());
}

bool CBattleEntity::CanRest()
{
    return !getMod(Mod::REGEN_DOWN) && !StatusEffectContainer->HasStatusEffectByFlag(EFFECTFLAG_NO_REST);
}

bool CBattleEntity::Rest(float rate)
{
    if (health.hp != health.maxhp || health.mp != health.maxmp)
    {
        // recover 20% HP
        uint32 recoverHP = (uint32)(health.maxhp * rate);
        uint32 recoverMP = (uint32)(health.maxmp * rate);
        addHP(recoverHP);
        addMP(recoverMP);

        // lower TP
        addTP((int16)(rate * -500));
        return true;
    }

    return false;
}

int16 CBattleEntity::GetWeaponDelay(bool tp)
{
    TracyZoneScoped;
    if (StatusEffectContainer->HasStatusEffect(EFFECT_HUNDRED_FISTS) && !tp)
    {
        return 1700;
    }
    uint16 WeaponDelay = 9999;
    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]))
    {
        uint16 MinimumDelay = weapon->getDelay(); // Track base delay.  We will need this later.  Mod::DELAY is ignored for now.
        WeaponDelay         = weapon->getDelay() - getMod(Mod::DELAY);
        if (weapon->isHandToHand())
        {
            WeaponDelay -= getMod(Mod::MARTIAL_ARTS) * 1000 / 60;
        }
        else if (auto* subweapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_SUB]);
                 subweapon && subweapon->getDmgType() > DAMAGE_TYPE::NONE && subweapon->getDmgType() < DAMAGE_TYPE::HTH)
        {
            MinimumDelay += subweapon->getDelay();
            WeaponDelay += subweapon->getDelay();
            // apply dual wield delay reduction
            WeaponDelay = (uint16)(WeaponDelay * ((100.0f - getMod(Mod::DUAL_WIELD)) / 100.0f));
        }

        // apply haste and delay reductions that don't affect tp
        if (!tp)
        {
            // Cap haste at appropriate levels.
            int16 hasteMagic   = std::clamp<int16>(getMod(Mod::HASTE_MAGIC), -10000, 4375);  // 43.75% cap -- handle 100% slow for weakness
            int16 hasteAbility = std::clamp<int16>(getMod(Mod::HASTE_ABILITY), -2500, 2500); // 25% cap
            int16 hasteGear    = std::clamp<int16>(getMod(Mod::HASTE_GEAR), -2500, 2500);    // 25%

            bool specialAttackList = false;

            // Check if we are using a special attack list that should not be affected by attack speed debuffs
            // Example: Wyrm's flying auto attack speed should not be modified by debuffs.
            if (this->objtype == TYPE_MOB)
                if (((CMobEntity*)this)->getMobMod(MOBMODIFIER::MOBMOD_ATTACK_SKILL_LIST) != 0)
                    specialAttackList = true;

            // Divide by float to get a more accurate reduction, then use int16 cast to truncate
            if (!specialAttackList)
                WeaponDelay -= (int16)(WeaponDelay * (hasteMagic + hasteAbility + hasteGear) / 10000.f);
        }
        WeaponDelay = (uint16)(WeaponDelay * ((100.0f + getMod(Mod::DELAYP)) / 100.0f));

        // Global delay reduction cap of "about 80%" being enforced.
        // This should be enforced on -delay equipment, martial arts, dual wield, and haste, hence MinimumDelay * 0.2.
        // TODO: Could be converted to value/1024 if the exact cap is ever determined.
        MinimumDelay -= (uint16)(MinimumDelay * 0.8);
        WeaponDelay = (WeaponDelay < MinimumDelay) ? MinimumDelay : WeaponDelay;
    }
    return WeaponDelay;
}

float CBattleEntity::GetMeleeRange() const
{
    return m_ModelRadius + 3.0f;
}

int16 CBattleEntity::GetRangedWeaponDelay(bool tp)
{
    CItemWeapon* PRange = (CItemWeapon*)m_Weapons[SLOT_RANGED];
    CItemWeapon* PAmmo  = (CItemWeapon*)m_Weapons[SLOT_AMMO];

    // base delay
    int16 delay = 0;

    if (PRange != nullptr && PRange->getDamage() != 0)
    {
        delay = ((PRange->getDelay() * 60) / 1000);
    }

    delay = (((delay - getMod(Mod::RANGED_DELAY)) * 1000) / 120);

    // apply haste and delay reductions that don't affect tp
    if (!tp)
    {
        delay = (int16)(delay * ((100.0f + getMod(Mod::RANGED_DELAYP)) / 100.0f));
    }
    else if (PAmmo)
    {
        delay += PAmmo->getDelay() / 2;
    }

    if (PAmmo != nullptr && PAmmo->isThrowing() && PAmmo->getDamage() != 0)
    {
        delay = PAmmo->getDelay() / 2;
    }

    return delay;
}

int16 CBattleEntity::GetAmmoDelay()
{
    CItemWeapon* PAmmo = (CItemWeapon*)m_Weapons[SLOT_AMMO];

    int delay = 0;
    if (PAmmo != nullptr && PAmmo->getDamage() != 0)
    {
        delay = PAmmo->getDelay() / 2;
    }

    return delay;
}

uint16 CBattleEntity::GetMainWeaponDmg()
{
    TracyZoneScoped;
    if ((objtype == TYPE_PET && static_cast<CPetEntity*>(this)->getPetType() == PET_TYPE::JUG_PET) || objtype == TYPE_MOB)
    {
        return mobutils::GetWeaponDamage(static_cast<CMobEntity*>(this), SLOT_MAIN);
    }

    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]))
    {
        if ((weapon->getReqLvl() > GetMLevel()) && objtype == TYPE_PC)
        {
            // TODO: Determine the difference between augments and latents w.r.t. equipment scaling.
            // MAIN_DMG_RATING already has equipment scaling applied elsewhere.
            uint16 dmg = weapon->getDamage();
            dmg *= GetMLevel() * 3;
            dmg /= 4;
            dmg /= weapon->getReqLvl();
            return dmg + weapon->getModifier(Mod::DMG_RATING) + getMod(Mod::MAIN_DMG_RATING);
        }
        else
        {
            return weapon->getDamage() + weapon->getModifier(Mod::DMG_RATING) + getMod(Mod::MAIN_DMG_RATING);
        }
    }
    return 0;
}

uint16 CBattleEntity::GetSubWeaponDmg()
{
    TracyZoneScoped;
    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_SUB]))
    {
        if ((weapon->getReqLvl() > GetMLevel()) && objtype == TYPE_PC)
        {
            uint16 dmg = weapon->getDamage();
            dmg *= GetMLevel() * 3;
            dmg /= 4;
            dmg /= weapon->getReqLvl();
            return dmg + weapon->getModifier(Mod::DMG_RATING) + getMod(Mod::SUB_DMG_RATING);
        }
        else
        {
            return weapon->getDamage() + weapon->getModifier(Mod::DMG_RATING) + getMod(Mod::SUB_DMG_RATING);
        }
    }
    return 0;
}

uint16 CBattleEntity::GetRangedWeaponDmg()
{
    TracyZoneScoped;
    uint16 dmg = 0;

    if (objtype == TYPE_MOB)
    {
        auto* PMob = static_cast<CMobEntity*>(this);
        return (mobutils::GetWeaponDamage(PMob, SLOT_RANGED) + getMod(Mod::RANGED_DMG_RATING)) * battleutils::GetRangedDistanceCorrection(this, distance(this->loc.p, this->GetBattleTarget()->loc.p), false);
    }

    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_RANGED]))
    {
        if ((weapon->getReqLvl() > GetMLevel()) && objtype == TYPE_PC)
        {
            uint16 scaleddmg = weapon->getDamage();
            scaleddmg *= GetMLevel() * 3;
            scaleddmg /= 4;
            scaleddmg /= weapon->getReqLvl();
            dmg += scaleddmg + weapon->getModifier(Mod::DMG_RATING);
        }
        else
        {
            dmg += weapon->getDamage() + weapon->getModifier(Mod::DMG_RATING);
        }
    }
    if (auto* ammo = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_AMMO]))
    {
        if ((ammo->getReqLvl() > GetMLevel()) && objtype == TYPE_PC)
        {
            uint16 scaleddmg = ammo->getDamage();
            scaleddmg *= GetMLevel() * 3;
            scaleddmg /= 4;
            scaleddmg /= ammo->getReqLvl();
            dmg += scaleddmg + ammo->getModifier(Mod::DMG_RATING);
        }
        else
        {
            dmg += ammo->getDamage() + ammo->getModifier(Mod::DMG_RATING);
        }
    }
    return dmg + getMod(Mod::RANGED_DMG_RATING);
}

uint16 CBattleEntity::GetMainWeaponRank()
{
    // https://www.bg-wiki.com/ffxi/Weapon_Rank
    // https://ffxiclopedia.fandom.com/wiki/Category:Hand-to-Hand?oldid=342430
    uint16 wDamage = 0;
    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]))
    {
        wDamage = weapon->getDamage() + getMod(Mod::MAIN_DMG_RANK);

        if (weapon->getSkillType() == SKILL_HAND_TO_HAND)
        {
            wDamage += 3;
        }
    }
    return wDamage / 9;
}

uint16 CBattleEntity::GetSubWeaponRank()
{
    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_SUB]))
    {
        return (weapon->getDamage() + getMod(Mod::SUB_DMG_RANK)) / 9;
    }
    return 0;
}

uint16 CBattleEntity::GetRangedWeaponRank()
{
    if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_RANGED]))
    {
        return (weapon->getDamage() + getMod(Mod::RANGED_DMG_RANK)) / 9;
    }
    return 0;
}

/************************************************************************
 *                                                                       *
 *  Изменяем количество TP сущности                                      *
 *                                                                       *
 ************************************************************************/

int16 CBattleEntity::addTP(int16 tp)
{
    // When adding TP, we must adjust for Inhibit TP effect, which reduces TP gain.
    if (tp > 0)
    {
        float tpReducePercent = this->getMod(Mod::INHIBIT_TP) / 100.0f;
        tp                    = (int16)(tp - (tp * tpReducePercent));

        float TPMulti = 1.0;

        if (objtype == TYPE_PC)
        {
            TPMulti = settings::get<float>("map.PLAYER_TP_MULTIPLIER");
        }
        else if (objtype == TYPE_PET || (objtype == TYPE_MOB && this->PMaster)) // normal pet or charmed pet
        {
            TPMulti = settings::get<float>("map.PET_TP_MULTIPLIER");
        }
        else if (objtype == TYPE_MOB)
        {
            TPMulti = settings::get<float>("map.MOB_TP_MULTIPLIER");
        }
        else if (objtype == TYPE_TRUST)
        {
            TPMulti = settings::get<float>("map.TRUST_TP_MULTIPLIER");
        }
        else if (objtype == TYPE_FELLOW)
        {
            TPMulti = settings::get<float>("map.FELLOW_TP_MULTIPLIER");
        }

        tp = (int16)(tp * TPMulti);
    }
    if (tp != 0)
    {
        updatemask |= UPDATE_HP;
    }
    int16 cap = std::clamp(health.tp + tp, 0, 3000);
    tp        = health.tp - cap;
    health.tp = cap;
    return abs(tp);
}

/************************************************************************
 *                                                                      *
 *  Изменяем количество жизней сущности                                 *
 *                                                                      *
 ************************************************************************/

int32 CBattleEntity::addHP(int32 hp)
{
    if (health.hp == 0 && hp < 0)
    {
        return 0; // if the entity is already dead, skip the rest to prevent killing it again
    }

    int32 cap = std::clamp(health.hp + hp, 0, GetMaxHP());
    hp        = health.hp - cap;
    health.hp = cap;

    // если количество жизней достигает нуля, то сущность умирает

    if (hp > 0)
    {
        battleutils::MakeEntityStandUp(this);
    }

    if (hp != 0)
    {
        updatemask |= UPDATE_HP;
    }

    if (health.hp == 0 && m_unkillable)
    {
        health.hp = 1;
    }

    return abs(hp);
}

int32 CBattleEntity::addMP(int32 mp)
{
    int32 cap = std::clamp(health.mp + mp, 0, GetMaxMP());
    mp        = health.mp - cap;
    health.mp = cap;
    if (mp != 0)
    {
        updatemask |= UPDATE_HP;
    }
    return abs(mp);
}

int32 CBattleEntity::takeDamage(int32 amount, CBattleEntity* attacker /* = nullptr*/, ATTACK_TYPE attackType /* = ATTACK_NONE*/,
                                DAMAGE_TYPE damageType /* = DAMAGE_NONE*/)
{
    TracyZoneScoped;

    if (this->GetLocalVar("DAMAGE_NULL") == 1)
    {
        amount %= 2;
        this->SetLocalVar("DAMAGE_DEALT", amount);
    }

    PLastAttacker                             = attacker;
    this->BattleHistory.lastHitTaken_atkType  = attackType;
    std::optional<CLuaBaseEntity> optAttacker = attacker ? std::optional<CLuaBaseEntity>(CLuaBaseEntity(attacker)) : std::nullopt;
    PAI->EventHandler.triggerListener("TAKE_DAMAGE", CLuaBaseEntity(this), amount, optAttacker, (uint16)attackType, (uint16)damageType);

    // RoE Damage Taken Trigger
    if (this->objtype == TYPE_PC)
    {
        if (amount > 0)
        {
            roeutils::event(ROE_EVENT::ROE_DMGTAKEN, static_cast<CCharEntity*>(this), RoeDatagram("dmg", amount));

            auto PChar = static_cast<CCharEntity*>(this);

            if (PChar && PChar->isInEvent() && PChar->currentEvent->type == EVENT_TYPE::MENU)
            {
                charutils::releaseEvent(PChar, false);
            }
        }
    }
    else if (PLastAttacker && PLastAttacker->objtype == TYPE_PC)
    {
        if (amount > 0)
        {
            roeutils::event(ROE_EVENT::ROE_DMGDEALT, static_cast<CCharEntity*>(attacker), RoeDatagram("dmg", amount));
        }

        // Took dmg from non ws source, so remove ws kill var
        this->SetLocalVar("weaponskillHit", 0);
    }

    if (getMod(Mod::ABSORB_DMG_TO_MP) > 0)
    {
        int16 absorbedMP = (int16)(amount * getMod(Mod::ABSORB_DMG_TO_MP) / 100);
        if (absorbedMP > 0)
        {
            addMP(absorbedMP);
        }
    }

    return addHP(-amount);
}

/************************************************************************
 *                                                                       *
 *  Полные значения характеристик боевой сущности                        *
 *                                                                       *
 ************************************************************************/

uint16 CBattleEntity::STR()
{
    return std::clamp(stats.STR + m_modStat[Mod::STR], 0, 999);
}

uint16 CBattleEntity::DEX()
{
    return std::clamp(stats.DEX + m_modStat[Mod::DEX], 0, 999);
}

uint16 CBattleEntity::VIT()
{
    return std::clamp(stats.VIT + m_modStat[Mod::VIT], 0, 999);
}

uint16 CBattleEntity::AGI()
{
    return std::clamp(stats.AGI + m_modStat[Mod::AGI], 0, 999);
}

uint16 CBattleEntity::INT()
{
    return std::clamp(stats.INT + m_modStat[Mod::INT], 0, 999);
}

uint16 CBattleEntity::MND()
{
    return std::clamp(stats.MND + m_modStat[Mod::MND], 0, 999);
}

uint16 CBattleEntity::CHR()
{
    return std::clamp(stats.CHR + m_modStat[Mod::CHR], 0, 999);
}

uint16 CBattleEntity::ATT(uint16 slot, bool ignoreWeaponMods)
{
    TracyZoneScoped;
    XI_DEBUG_BREAK_IF(slot >= sizeof(m_Weapons) / sizeof(CItemEquipment*));

    // TODO: consider which weapon!
    int32 ATT    = 8 + m_modStat[Mod::ATT];
    auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[slot]);
    if (weapon && weapon->isTwoHanded())
    {
        ATT += (STR() * 3) / 4;
    }
    else
    {
        ATT += STR() / 2;
    }

    if (this->StatusEffectContainer->HasStatusEffect(EFFECT_ENDARK))
    {
        ATT += this->getMod(Mod::ENSPELL_DMG);
    }

    if (this->objtype & TYPE_PC)
    {
        if (weapon)
        {
            ATT += GetSkill(weapon->getSkillType()) + weapon->getILvlSkill();

            if (!ignoreWeaponMods)
            {
                if (weapon->getModifier(Mod::ATT_SLOT) > 0)
                {
                    ATT += weapon->getModifier(Mod::ATT_SLOT);
                }
            }

            // Smite applies when using 2H or H2H weapons
            if (weapon->isTwoHanded() || weapon->isHandToHand())
            {
                ATT += static_cast<int32>(ATT * this->getMod(Mod::SMITE) / 256.f); // Divide smite value by 256
            }
        }
    }
    else if (this->objtype == TYPE_PET && ((CPetEntity*)this)->getPetType() == PET_TYPE::AUTOMATON)
    {
        ATT += this->GetSkill(SKILL_AUTOMATON_MELEE);
    }
    else // Mob attack
    {
        ATT = (8 + m_modStat[Mod::ATT] + STR() / 2);
    }

    return std::clamp(ATT + (ATT * m_modStat[Mod::ATTP] / 100) + std::min<int16>((ATT * m_modStat[Mod::FOOD_ATTP] / 100), m_modStat[Mod::FOOD_ATT_CAP]), 0, 65535);
}

uint16 CBattleEntity::RATT(uint8 skill, float distance, uint16 bonusSkill)
{
    auto* PWeakness = StatusEffectContainer->GetStatusEffect(EFFECT_WEAKNESS);

    if (PWeakness && PWeakness->GetPower() >= 2)
    {
        return 0;
    }

    int32 ATT = 8 + GetSkill(skill == 48 ? 0 : skill) + bonusSkill + m_modStat[Mod::RATT] + battleutils::GetRangedAttackBonuses(this) + STR() / 2;

    if ((this->objtype == TYPE_PC) || (this->objtype == TYPE_PET && this->PMaster->objtype == TYPE_PC && ((CPetEntity*)this)->getPetType() == PET_TYPE::AUTOMATON)) // PC or PC Automaton
    {
        ATT = int32((float)ATT * battleutils::GetRangedDistanceCorrection(this, distance, true));
    }

    return std::clamp(ATT + (ATT * m_modStat[Mod::RATTP] / 100) + std::min<int16>((ATT * m_modStat[Mod::FOOD_RATTP] / 100), m_modStat[Mod::FOOD_RATT_CAP]), 0, 65535);
}

uint16 CBattleEntity::GetBaseRATT(uint8 skill, uint16 bonusSkill)
{
    auto* PWeakness = StatusEffectContainer->GetStatusEffect(EFFECT_WEAKNESS);

    if (PWeakness && PWeakness->GetPower() >= 2)
    {
        return 0;
    }

    int32 ATT = 8 + GetSkill(skill == 48 ? 0 : skill) + bonusSkill + m_modStat[Mod::RATT] + battleutils::GetRangedAttackBonuses(this) + (STR() * 3) / 4;

    return std::clamp(ATT + (ATT * m_modStat[Mod::RATTP] / 100) + std::min<int16>((ATT * m_modStat[Mod::FOOD_RATTP] / 100), m_modStat[Mod::FOOD_RATT_CAP]), 0, 65535);
}

uint16 CBattleEntity::RACC(uint8 skill, float distance, uint16 bonusSkill)
{
    TracyZoneScoped;
    auto* PWeakness = StatusEffectContainer->GetStatusEffect(EFFECT_WEAKNESS);

    if (PWeakness && PWeakness->GetPower() >= 2)
    {
        return 0;
    }

    int16 skill_level = GetSkill(skill == 48 ? 0 : skill) + bonusSkill;
    int16 acc         = skill_level;

    if (skill_level > 200)
    {
        acc = (int16)(200 + (skill_level - 200) * 0.9);
    }

    acc += getMod(Mod::RACC);
    acc += battleutils::GetRangedAccuracyBonuses(this);
    acc += AGI() / 2;

    if ((this->objtype == TYPE_PC) || (this->objtype == TYPE_PET && this->PMaster->objtype == TYPE_PC && ((CPetEntity*)this)->getPetType() == PET_TYPE::AUTOMATON)) // PC or PC Automaton
    {
        if (!this->StatusEffectContainer->HasStatusEffect(EFFECT_SHARPSHOT))
        {
            acc = int16((float)acc * battleutils::GetRangedDistanceCorrection(this, distance, false));
        }
    }

    return std::clamp(acc + std::min<int16>(((100 + getMod(Mod::FOOD_RACCP) * acc) / 100), getMod(Mod::FOOD_RACC_CAP)), 0, 65535);
}

uint16 CBattleEntity::GetBaseRACC(uint8 skill, uint16 bonusSkill)
{
    TracyZoneScoped;
    auto* PWeakness = StatusEffectContainer->GetStatusEffect(EFFECT_WEAKNESS);

    if (PWeakness && PWeakness->GetPower() >= 2)
    {
        return 0;
    }

    int16 skill_level = GetSkill(skill == 48 ? 0 : skill) + bonusSkill;
    int16 acc         = skill_level;

    if (skill_level > 200)
    {
        acc = (int16)(200 + (skill_level - 200) * 0.9);
    }

    acc += getMod(Mod::RACC);
    acc += battleutils::GetRangedAccuracyBonuses(this);
    acc += AGI() / 2;

    return std::clamp(acc + std::min<int16>(((100 + getMod(Mod::FOOD_RACCP) * acc) / 100), getMod(Mod::FOOD_RACC_CAP)), 0, 65535);
}

uint16 CBattleEntity::ACC(uint8 attackNumber, int8 offsetAccuracy)
{
    TracyZoneScoped;
    if (this->objtype & TYPE_PC)
    {
        uint8  skill     = 0;
        uint16 iLvlSkill = 0;
        if (attackNumber == 0)
        {
            if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]))
            {
                skill     = weapon->getSkillType();
                iLvlSkill = weapon->getILvlSkill();
                if (skill == SKILL_NONE && GetSkill(SKILL_HAND_TO_HAND) > 0)
                {
                    skill = SKILL_HAND_TO_HAND;
                }
            }
        }
        else if (attackNumber == 1)
        {
            if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_SUB]))
            {
                skill     = weapon->getSkillType();
                iLvlSkill = weapon->getILvlSkill();
                if (skill == SKILL_NONE && GetSkill(SKILL_HAND_TO_HAND) > 0)
                {
                    auto* main_weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]);
                    if (main_weapon && (main_weapon->getSkillType() == SKILL_NONE || main_weapon->getSkillType() == SKILL_HAND_TO_HAND))
                    {
                        skill = SKILL_HAND_TO_HAND;
                    }
                }
            }
        }
        else if (attackNumber == 2)
        {
            if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]))
            {
                iLvlSkill = weapon->getILvlSkill();
            }
            skill = SKILL_HAND_TO_HAND;
        }
        int16 ACC = GetSkill(skill) + iLvlSkill;
        ACC       = (ACC > 200 ? (int16)(((ACC - 200) * 0.9) + 200) : ACC);
        if (auto* weapon = dynamic_cast<CItemWeapon*>(m_Weapons[SLOT_MAIN]); weapon && weapon->isTwoHanded())
        {
            ACC += (int16)(DEX() * 0.75);
        }
        else
        {
            ACC += (int16)(DEX() * 0.5);
        }
        ACC         = (ACC + m_modStat[Mod::ACC] + offsetAccuracy);
        auto* PChar = dynamic_cast<CCharEntity*>(this);
        if (PChar)
        {
            ACC += PChar->PMeritPoints->GetMeritValue(MERIT_ACCURACY, PChar);
        }
        ACC = ACC + std::min<int16>((ACC * m_modStat[Mod::FOOD_ACCP] / 100), m_modStat[Mod::FOOD_ACC_CAP]);
        return std::max<int16>(0, ACC);
    }
    else if (this->objtype == TYPE_PET && ((CPetEntity*)this)->getPetType() == PET_TYPE::AUTOMATON)
    {
        int16 ACC = this->GetSkill(SKILL_AUTOMATON_MELEE);
        ACC       = (ACC > 200 ? (int16)(((ACC - 200) * 0.9) + 200) : ACC);
        ACC += (int16)(DEX() * 0.5);
        ACC += m_modStat[Mod::ACC] + offsetAccuracy;
        ACC = ACC + std::min<int16>((ACC * m_modStat[Mod::FOOD_ACCP] / 100), m_modStat[Mod::FOOD_ACC_CAP]);
        return std::max<int16>(0, ACC);
    }
    else
    {
        int16 ACC = m_modStat[Mod::ACC];
        ACC       = ACC + std::min<int16>((ACC * m_modStat[Mod::FOOD_ACCP] / 100), m_modStat[Mod::FOOD_ACC_CAP]) + DEX() / 2; // food mods here for Snatch Morsel
        return std::max<int16>(0, ACC);
    }
}

uint16 CBattleEntity::DEF()
{
    int32 DEF = 8 + m_modStat[Mod::DEF] + VIT() / 2;

    if (this->StatusEffectContainer->HasStatusEffect(EFFECT_COUNTERSTANCE, 0))
    {
        DEF = 1 + VIT() / 2;

        if (this->StatusEffectContainer->HasStatusEffect(EFFECT_MINNE))
        {
            DEF += this->StatusEffectContainer->GetStatusEffect(EFFECT_MINNE)->GetPower();
        }

        int32 defModApplicatble = 0;

        defModApplicatble += std::clamp((DEF * std::clamp((int)m_modStat[Mod::DEFP], -100, 0) / 100), -999, 0);

        return std::clamp(DEF + defModApplicatble, 0, 65535);
    }

    return std::clamp(DEF + (DEF * m_modStat[Mod::DEFP] / 100) + std::min<int16>((DEF * m_modStat[Mod::FOOD_DEFP] / 100), m_modStat[Mod::FOOD_DEF_CAP]), 0, 65535);
}

uint16 CBattleEntity::EVA()
{
    int16 evasion = GetSkill(SKILL_EVASION);

    // This check is for Players Only
    if (evasion > 200)
    { // Evasion skill is 0.9 evasion post-200
        evasion = (int16)(200 + (evasion - 200) * 0.9);
    }

    // Mobs do not have SKILL_EVASION. Their stats are set in the mobutils.cpp. This will correctly set their evasion
    if (this->objtype == TYPE_MOB || this->objtype == TYPE_PET)
    {
        int16 evasionMob = (200 + (m_modStat[Mod::EVA] - 200) * 0.9);
        return std::max(0, (evasionMob + AGI() / 2));
    }

    return std::max(0, (m_modStat[Mod::EVA] + evasion + AGI() / 2));
}

/************************************************************************
 *                                                                       *
 *                                                                       *
 *                                                                       *
 ************************************************************************/

JOBTYPE CBattleEntity::GetMJob()
{
    return m_mjob;
}

uint8 CBattleEntity::GetMLevel() const
{
    return m_mlvl;
}

JOBTYPE CBattleEntity::GetSJob()
{
    if (StatusEffectContainer->HasStatusEffect({ EFFECT_OBLIVISCENCE, EFFECT_SJ_RESTRICTION }))
    {
        return JOB_NON;
    }
    return m_sjob;
}

uint8 CBattleEntity::GetSLevel() const
{
    if (StatusEffectContainer->HasStatusEffect({ EFFECT_OBLIVISCENCE, EFFECT_SJ_RESTRICTION }))
    {
        return 0;
    }
    return m_slvl;
}

void CBattleEntity::SetMJob(uint8 mjob)
{
    XI_DEBUG_BREAK_IF(mjob == 0 || mjob >= MAX_JOBTYPE); // выход за пределы доступных профессий

    m_mjob = (JOBTYPE)mjob;
}

void CBattleEntity::SetSJob(uint8 sjob)
{
    XI_DEBUG_BREAK_IF(sjob >= MAX_JOBTYPE); // выход за пределы доступных профессий

    m_sjob = (JOBTYPE)sjob;
}

void CBattleEntity::SetMLevel(uint8 mlvl)
{
    TracyZoneScoped;
    m_modStat[Mod::DEF] -= m_mlvl + std::clamp(m_mlvl - 50, 0, 10);
    m_mlvl = (mlvl == 0 ? 1 : mlvl);
    m_modStat[Mod::DEF] += m_mlvl + std::clamp(m_mlvl - 50, 0, 10);

    if (this->objtype & TYPE_PC)
    {
        sql->Query("UPDATE char_stats SET mlvl = %u WHERE charid = %u LIMIT 1;", m_mlvl, this->id);
    }
}

void CBattleEntity::SetSLevel(uint8 slvl)
{
    TracyZoneScoped;
    if (!settings::get<bool>("map.INCLUDE_MOB_SJ") && this->objtype == TYPE_MOB && this->objtype != TYPE_PET)
    {
        m_slvl = m_mlvl; // All mobs have a 1:1 ratio of MainJob/Subjob
    }
    else if (this->objtype == TYPE_PET &&
             (static_cast<CPetEntity*>(this)->getPetType() == PET_TYPE::AVATAR || static_cast<CPetEntity*>(this)->getPetType() == PET_TYPE::AVATAR) &&
             static_cast<CPetEntity*>(this)->PMaster != nullptr && static_cast<CPetEntity*>(this)->PMaster->objtype == TYPE_PC)
    {
        m_slvl = this->GetMLevel();
    }
    else if (this->objtype == TYPE_PET &&
             (static_cast<CPetEntity*>(this)->getPetType() == PET_TYPE::AVATAR || static_cast<CPetEntity*>(this)->m_PetID <= PETID_DARKSPIRIT) &&
             static_cast<CPetEntity*>(this)->PMaster != nullptr && static_cast<CPetEntity*>(this)->PMaster->objtype == TYPE_PC)
    {
        m_slvl = this->GetMLevel();
    }
    else
    {
        auto ratio = settings::get<uint8>("map.SUBJOB_RATIO");
        switch (ratio)
        {
            case 0: // no SJ
                m_slvl = 0;
                break;
            case 1: // 1/2 (75/37, 99/49)
                m_slvl = (slvl > (m_mlvl >> 1) ? (m_mlvl == 1 ? 1 : (m_mlvl >> 1)) : slvl);
                break;
            case 2: // 2/3 (75/50, 99/66)
                m_slvl = (slvl > (m_mlvl * 2) / 3 ? (m_mlvl == 1 ? 1 : (m_mlvl * 2) / 3) : slvl);
                break;
            case 3: // equal (75/75, 99/99)
                m_slvl = (slvl > m_mlvl ? (m_mlvl == 1 ? 1 : m_mlvl) : slvl);
                break;
            default: // Error
                ShowError("Error setting subjob level: Invalid ratio '%s' check your settings file!", ratio);
                break;
        }
    }

    if (this->objtype & TYPE_PC)
    {
        sql->Query("UPDATE char_stats SET slvl = %u WHERE charid = %u LIMIT 1;", m_slvl, this->id);
    }
}

void CBattleEntity::SetDeathType(uint8 type)
{
    m_DeathType = static_cast<DEATH_TYPE>(type);
}

uint8 CBattleEntity::GetDeathType()
{
    return static_cast<uint8>(m_DeathType);
}

/************************************************************************
 *                                                                      *
 *  Добавляем модификатор                                               *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::addModifier(Mod type, int16 amount)
{
    if (type == Mod::MOVE)
    {
        m_MSNonItemValues.push_back(amount);
        m_modStat[type] = CalculateMSFromSources();
    }
    else
    {
        m_modStat[type] += amount;
    }
}

/************************************************************************
 *                                                                      *
 *  Добавляем модификаторы                                              *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::addModifiers(std::vector<CModifier>* modList)
{
    TracyZoneScoped;
    for (auto modifier : *modList)
    {
        if (modifier.getModID() == Mod::MOVE)
        {
            addModifier(modifier.getModID(), modifier.getModAmount()); // Calculations in addModifier already, don't duplicate
        }
        else
        {
            m_modStat[modifier.getModID()] += modifier.getModAmount();
        }
    }
}

int16 CBattleEntity::CalculateMSFromSources()
{
    int16 highestItemPositiveValue = 0;
    int16 highestNonItemPositve    = 0;
    int16 totalItemReducedValue    = 0;
    int16 totalNonItemReducedValue = 0;

    for (uint16 i = 0; i < m_MSNonItemValues.size(); i++)
    {
        if (m_MSNonItemValues[i] >= 0)
        {
            if (m_MSNonItemValues[i] > highestNonItemPositve)
            {
                highestNonItemPositve = m_MSNonItemValues[i];
            }
        }
        else
        {
            totalNonItemReducedValue += m_MSNonItemValues[i];
        }
    }

    for (uint16 i = 0; i < m_MSItemValues.size(); i++)
    {
        if (m_MSItemValues[i] >= 0)
        {
            if (m_MSItemValues[i] > highestItemPositiveValue)
            {
                highestItemPositiveValue = m_MSItemValues[i];
            }
        }
        else
        {
            totalItemReducedValue += m_MSItemValues[i];
        }
    }

    return (highestItemPositiveValue + totalItemReducedValue) + (highestNonItemPositve + totalNonItemReducedValue);
}

void CBattleEntity::addEquipModifiers(std::vector<CModifier>* modList, uint8 itemLevel, uint8 slotid)
{
    TracyZoneScoped;
    if (GetMLevel() >= itemLevel)
    {
        for (auto& i : *modList)
        {
            if (slotid == SLOT_SUB)
            {
                if (i.getModID() == Mod::MAIN_DMG_RANK)
                {
                    m_modStat[Mod::SUB_DMG_RANK] += i.getModAmount();
                }
                else
                {
                    m_modStat[i.getModID()] += i.getModAmount();
                }
            }
            else
            {
                // Add item with movement speed and prevent stacking
                if (i.getModID() == Mod::MOVE)
                {
                    m_MSItemValues.push_back(i.getModAmount());
                    m_modStat[i.getModID()] = CalculateMSFromSources();
                }
                else
                {
                    m_modStat[i.getModID()] += i.getModAmount();
                }
            }
        }
    }
    else
    {
        for (auto& i : *modList)
        {
            int16 modAmount = GetMLevel() * i.getModAmount();
            switch (i.getModID())
            {
                case Mod::DEF:
                case Mod::MAIN_DMG_RATING:
                case Mod::SUB_DMG_RATING:
                case Mod::RANGED_DMG_RATING:
                    modAmount *= 3;
                    modAmount /= 4;
                    break;
                case Mod::HP:
                case Mod::MP:
                    modAmount /= 2;
                    break;
                case Mod::STR:
                case Mod::DEX:
                case Mod::VIT:
                case Mod::AGI:
                case Mod::INT:
                case Mod::MND:
                case Mod::CHR:
                case Mod::ATT:
                case Mod::RATT:
                case Mod::ACC:
                case Mod::RACC:
                case Mod::MATT:
                case Mod::MACC:
                    modAmount /= 3;
                    break;
                default:
                    modAmount = 0;
                    break;
            }
            modAmount /= itemLevel;
            if (slotid == SLOT_SUB)
            {
                if (i.getModID() == Mod::MAIN_DMG_RANK)
                {
                    m_modStat[Mod::SUB_DMG_RANK] += modAmount;
                }
                else
                {
                    m_modStat[i.getModID()] += modAmount;
                }
            }
            else
            {
                m_modStat[i.getModID()] += modAmount;
            }
        }
    }
}

/************************************************************************
 *                                                                      *
 *  Устанавливаем модификатор                                           *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::setModifier(Mod type, int16 amount)
{
    m_modStat[type] = amount;
}

/************************************************************************
 *                                                                      *
 *  Устанавливаем модификаторы                                          *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::setModifiers(std::vector<CModifier>* modList)
{
    TracyZoneScoped;
    for (auto& i : *modList)
    {
        m_modStat[i.getModID()] = i.getModAmount();
    }
}

/************************************************************************
 *                                                                      *
 *  Удаляем модификатор                                                 *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::delModifier(Mod type, int16 amount)
{
    if (type == Mod::MOVE)
    {
        for (uint16 x = 0; x < m_MSNonItemValues.size(); x++)
        {
            if (m_MSNonItemValues[x] == amount)
            {
                m_MSNonItemValues.erase(m_MSNonItemValues.begin() + x);
                break;
            }
        }

        m_modStat[type] = CalculateMSFromSources();
    }
    else
    {
        m_modStat[type] -= amount;
    }
}

void CBattleEntity::saveModifiers()
{
    m_modStatSave = m_modStat;
}

void CBattleEntity::restoreModifiers()
{
    m_modStat = m_modStatSave;
}

/************************************************************************
 *                                                                      *
 *  Удаляем модификаторы                                                *
 *                                                                      *
 ************************************************************************/

void CBattleEntity::delModifiers(std::vector<CModifier>* modList)
{
    TracyZoneScoped;
    for (auto& i : *modList)
    {
        if (i.getModID() == Mod::MOVE)
        {
            delModifier(i.getModID(), i.getModAmount()); // Calculations in delModifier already, don't duplicate
        }
        else
        {
            m_modStat[i.getModID()] -= i.getModAmount();
        }
    }
}

void CBattleEntity::delEquipModifiers(std::vector<CModifier>* modList, uint8 itemLevel, uint8 slotid)
{
    TracyZoneScoped;
    if (GetMLevel() >= itemLevel)
    {
        for (auto& i : *modList)
        {
            if (slotid == SLOT_SUB)
            {
                if (i.getModID() == Mod::MAIN_DMG_RANK)
                {
                    m_modStat[Mod::SUB_DMG_RANK] -= i.getModAmount();
                }
                else
                {
                    m_modStat[i.getModID()] -= i.getModAmount();
                }
            }
            else
            {
                // Remove item with movement speed and prevent stacking
                if (i.getModID() == Mod::MOVE)
                {
                    for (uint16 x = 0; x < m_MSItemValues.size(); x++)
                    {
                        if (m_MSItemValues[x] == i.getModAmount())
                        {
                            m_MSItemValues.erase(m_MSItemValues.begin() + x);
                            break;
                        }
                    }

                    m_modStat[i.getModID()] = CalculateMSFromSources();
                }
                else
                {
                    m_modStat[i.getModID()] -= i.getModAmount();
                }
            }
        }
    }
    else
    {
        for (auto& i : *modList)
        {
            int16 modAmount = GetMLevel() * i.getModAmount();
            switch (i.getModID())
            {
                case Mod::DEF:
                case Mod::MAIN_DMG_RATING:
                case Mod::SUB_DMG_RATING:
                case Mod::RANGED_DMG_RATING:
                    modAmount *= 3;
                    modAmount /= 4;
                    break;
                case Mod::HP:
                case Mod::MP:
                    modAmount /= 2;
                    break;
                case Mod::STR:
                case Mod::DEX:
                case Mod::VIT:
                case Mod::AGI:
                case Mod::INT:
                case Mod::MND:
                case Mod::CHR:
                case Mod::ATT:
                case Mod::RATT:
                case Mod::ACC:
                case Mod::RACC:
                case Mod::MATT:
                case Mod::MACC:
                    modAmount /= 3;
                    break;
                default:
                    modAmount = 0;
                    break;
            }
            modAmount /= itemLevel;
            if (slotid == SLOT_SUB)
            {
                if (i.getModID() == Mod::MAIN_DMG_RANK)
                {
                    m_modStat[Mod::SUB_DMG_RANK] -= modAmount;
                }
                else
                {
                    m_modStat[i.getModID()] -= modAmount;
                }
            }
            else
            {
                m_modStat[i.getModID()] -= modAmount;
            }
        }
    }
}

/************************************************************************
 *                                                                      *
 *  Получаем текущее значение указанного модификатора                   *
 *                                                                      *
 ************************************************************************/

int16 CBattleEntity::getMod(Mod modID)
{
    TracyZoneScoped;
    return m_modStat[modID];
}

void CBattleEntity::addPetModifier(Mod type, PetModType petmod, int16 amount)
{
    TracyZoneScoped;
    m_petMod[petmod][type] += amount;

    if (PPet && petutils::CheckPetModType(PPet, petmod))
    {
        PPet->addModifier(type, amount);
        PPet->UpdateHealth();
    }
}

void CBattleEntity::setPetModifier(Mod type, PetModType petmod, int16 amount)
{
    TracyZoneScoped;
    m_petMod[petmod][type] = amount;

    if (PPet && petutils::CheckPetModType(PPet, petmod))
    {
        PPet->setModifier(type, amount);
        PPet->UpdateHealth();
    }
}

void CBattleEntity::delPetModifier(Mod type, PetModType petmod, int16 amount)
{
    TracyZoneScoped;
    m_petMod[petmod][type] -= amount;

    if (PPet && petutils::CheckPetModType(PPet, petmod))
    {
        PPet->delModifier(type, amount);
        PPet->UpdateHealth();
    }
}

void CBattleEntity::addPetModifiers(std::vector<CPetModifier>* modList)
{
    TracyZoneScoped;
    for (auto modifier : *modList)
    {
        addPetModifier(modifier.getModID(), modifier.getPetModType(), modifier.getModAmount());
    }
}

void CBattleEntity::delPetModifiers(std::vector<CPetModifier>* modList)
{
    TracyZoneScoped;
    for (auto modifier : *modList)
    {
        delPetModifier(modifier.getModID(), modifier.getPetModType(), modifier.getModAmount());
    }
}

void CBattleEntity::applyPetModifiers(CPetEntity* PPet)
{
    TracyZoneScoped;
    for (const auto& modtype : m_petMod)
    {
        if (petutils::CheckPetModType(PPet, modtype.first))
        {
            for (auto mod : modtype.second)
            {
                PPet->addModifier(mod.first, mod.second);
                PPet->UpdateHealth();
            }
        }
    }
}

void CBattleEntity::removePetModifiers(CPetEntity* PPet)
{
    TracyZoneScoped;
    for (const auto& modtype : m_petMod)
    {
        if (petutils::CheckPetModType(PPet, modtype.first))
        {
            for (auto mod : modtype.second)
            {
                PPet->delModifier(mod.first, mod.second);
                PPet->UpdateHealth();
            }
        }
    }
}

/************************************************************************
 *                                                                      *
 *  Текущая величина умения (не максимальная, а ограниченная уровнем)   *
 *                                                                      *
 ************************************************************************/

uint16 CBattleEntity::GetSkill(uint16 SkillID)
{
    TracyZoneScoped;
    if (SkillID < MAX_SKILLTYPE)
    {
        return WorkingSkills.skill[SkillID] & 0x7FFF;
    }
    return 0;
}

void CBattleEntity::addTrait(CTrait* PTrait)
{
    TracyZoneScoped;
    TraitList.push_back(PTrait);
    addModifier(PTrait->getMod(), PTrait->getValue());
}

void CBattleEntity::delTrait(CTrait* PTrait)
{
    TracyZoneScoped;
    delModifier(PTrait->getMod(), PTrait->getValue());
    TraitList.erase(std::remove(TraitList.begin(), TraitList.end(), PTrait), TraitList.end());
}

bool CBattleEntity::ValidTarget(CBattleEntity* PInitiator, uint16 targetFlags)
{
    TracyZoneScoped;
    if (targetFlags & TARGET_ENEMY)
    {
        if (!isDead())
        {
            // Teams PVP
            if (allegiance >= ALLEGIANCE_TYPE::WYVERNS && PInitiator->allegiance >= ALLEGIANCE_TYPE::WYVERNS)
            {
                return allegiance != PInitiator->allegiance;
            }

            // Nation PVP
            if ((allegiance >= ALLEGIANCE_TYPE::SAN_DORIA && allegiance <= ALLEGIANCE_TYPE::WINDURST) &&
                (PInitiator->allegiance >= ALLEGIANCE_TYPE::SAN_DORIA && PInitiator->allegiance <= ALLEGIANCE_TYPE::WINDURST))
            {
                return allegiance != PInitiator->allegiance;
            }

            // PVE
            if (allegiance <= ALLEGIANCE_TYPE::PLAYER && PInitiator->allegiance <= ALLEGIANCE_TYPE::PLAYER)
            {
                return allegiance != PInitiator->allegiance;
            }

            return false;
        }
    }

    return (targetFlags & TARGET_SELF) &&
           (this == PInitiator ||
            (PInitiator->objtype == TYPE_PET && static_cast<CPetEntity*>(PInitiator)->getPetType() == PET_TYPE::AUTOMATON && this == PInitiator->PMaster));
}

bool CBattleEntity::CanUseSpell(CSpell* PSpell)
{
    TracyZoneScoped;
    return spell::CanUseSpell(this, PSpell);
}

void CBattleEntity::Spawn()
{
    TracyZoneScoped;
    animation = ANIMATION_NONE;
    HideName(false);
    CBaseEntity::Spawn();
    m_OwnerID.clean();
    setBattleID(0);
}

void CBattleEntity::Die()
{
    TracyZoneScoped;
    if (CBaseEntity* PKiller = GetEntity(m_OwnerID.targid))
    {
        // clang-format off
        static_cast<CBattleEntity*>(PKiller)->ForAlliance([this](CBattleEntity* PMember)
        {
            CCharEntity* member = static_cast<CCharEntity*>(PMember);
            if (member->PClaimedMob == this)
            {
                member->PClaimedMob = nullptr;
            }
        });
        // clang-format on
        PAI->EventHandler.triggerListener("DEATH", CLuaBaseEntity(this), CLuaBaseEntity(PKiller));
    }
    else
    {
        PAI->EventHandler.triggerListener("DEATH", CLuaBaseEntity(this));
    }
    SetBattleTargetID(0);
}

void CBattleEntity::OnDeathTimer()
{
    TracyZoneScoped;
}

void CBattleEntity::OnCastFinished(CMagicState& state, action_t& action)
{
    TracyZoneScoped;
    auto*          PSpell          = state.GetSpell();
    auto*          PActionTarget   = static_cast<CBattleEntity*>(state.GetTarget());
    CBattleEntity* POriginalTarget = PActionTarget;
    bool           IsMagicCovered  = false;

    // Check that the target id hasn't been released since starting the spell cast.
    if (!state.CheckTarget())
    {
        return;
    }

    luautils::OnSpellPrecast(this, PSpell);

    state.SpendCost();

    // remove effects based on spell cast first
    int16 effectFlags = EFFECTFLAG_INVISIBLE | EFFECTFLAG_MAGIC_BEGIN;

    if (PSpell->canTargetEnemy())
    {
        effectFlags |= EFFECTFLAG_DETECTABLE;
    }

    StatusEffectContainer->DelStatusEffectsByFlag(effectFlags);

    PAI->TargetFind->reset();

    // setup special targeting flags
    // can this spell target the dead?

    uint8 flags = FINDFLAGS_NONE;
    if (PSpell->getValidTarget() & TARGET_PLAYER_DEAD)
    {
        flags |= FINDFLAGS_DEAD;
    }
    if (PSpell->getFlag() & SPELLFLAG_HIT_ALL)
    {
        flags |= FINDFLAGS_HIT_ALL;
    }
    uint8      aoeType    = battleutils::GetSpellAoEType(this, PSpell);
    AOE_RADIUS radiusType = (PSpell->getFlag() & SPELLFLAG_ATTACKER_RADIUS) != 0 ? AOE_RADIUS::ATTACKER : AOE_RADIUS::TARGET;

    if (aoeType == SPELLAOE_RADIAL)
    {
        float distance = spell::GetSpellRadius(PSpell, this);

        PAI->TargetFind->findWithinArea(PActionTarget, radiusType, distance, flags);
    }
    else if (aoeType == SPELLAOE_CONAL)
    {
        // TODO: actual radius calculation
        float radius = spell::GetSpellRadius(PSpell, this);
        float angle  = spell::GetSpellConeAngle(PSpell, this);

        PAI->TargetFind->findWithinCone(PActionTarget, radiusType, radius, angle, flags);
    }
    else
    {
        if (this->objtype == TYPE_MOB && PActionTarget->objtype == TYPE_PC)
        {
            CBattleEntity* PCoverAbilityUser = battleutils::GetCoverAbilityUser(PActionTarget, this);
            IsMagicCovered                   = battleutils::IsMagicCovered((CCharEntity*)PCoverAbilityUser);

            if (IsMagicCovered)
            {
                PActionTarget = PCoverAbilityUser;
            }
        }
        // only add target
        PAI->TargetFind->findSingleTarget(PActionTarget, flags);
    }

    auto totalTargets = (uint16)PAI->TargetFind->m_targets.size();

    PSpell->setTotalTargets(totalTargets);
    PSpell->setPrimaryTargetID(PActionTarget->id);

    action.id         = id;
    action.actiontype = ACTION_MAGIC_FINISH;
    action.actionid   = static_cast<uint16>(PSpell->getID());
    action.recast     = state.GetRecast();
    action.spellgroup = PSpell->getSpellGroup();

    uint16 msg = MSGBASIC_NONE;

    for (auto* PTarget : PAI->TargetFind->m_targets)
    {
        actionList_t& actionList  = action.getNewActionList();
        actionList.ActionTargetID = PTarget->id;

        actionTarget_t& actionTarget = actionList.getNewActionTarget();

        actionTarget.reaction   = REACTION::NONE;
        actionTarget.speceffect = SPECEFFECT::NONE;
        actionTarget.animation  = PSpell->getAnimationID();
        actionTarget.param      = 0;
        actionTarget.messageID  = MSGBASIC_NONE;

        auto ce = PSpell->getCE();
        auto ve = PSpell->getVE();

        int32 damage = 0;

        // Take all shadows
        if (PSpell->canTargetEnemy() && (aoeType > SPELLAOE_NONE || (PSpell->getFlag() & SPELLFLAG_WIPE_SHADOWS)))
        {
            PTarget->StatusEffectContainer->DelStatusEffect(EFFECT_BLINK);
            PTarget->StatusEffectContainer->DelStatusEffect(EFFECT_COPY_IMAGE);
        }

        // TODO: this is really hacky and should eventually be moved into lua, and spellFlags should probably be in the spells table..
        if (PSpell->canHitShadow() && aoeType == SPELLAOE_NONE && battleutils::IsAbsorbByShadow(PTarget, this) && !(PSpell->getFlag() & SPELLFLAG_IGNORE_SHADOWS))
        {
            // take shadow
            msg                = MSGBASIC_SHADOW_ABSORB;
            actionTarget.param = 1;
            ve                 = 0;
            ce                 = 0;
        }
        else
        {
            damage = luautils::OnSpellCast(this, PTarget, PSpell);

            // Remove Saboteur
            if (PSpell->getSkillType() == SKILLTYPE::SKILL_ENFEEBLING_MAGIC)
            {
                StatusEffectContainer->DelStatusEffect(EFFECT_SABOTEUR);
            }

            if (msg == MSGBASIC_NONE)
            {
                msg = PSpell->getMessage();
            }
            else
            {
                msg = PSpell->getAoEMessage();
            }

            actionTarget.param    = damage;
            actionTarget.modifier = PSpell->getModifier();
            PSpell->setModifier(MODIFIER::NONE); // Reset modifier on use
        }

        if (actionTarget.animation == 122)
        { // Teleport spells don't target unqualified members
            if (PSpell->getMessage() == MSGBASIC_NONE)
            {
                actionTarget.animation = 0; // stop target from going invisible
                if (PTarget != PActionTarget)
                {
                    action.actionLists.pop_back();
                }
                else
                { // set this message in anticipation of nobody having the gate crystal
                    actionTarget.messageID = MSGBASIC_MAGIC_NO_EFFECT;
                }
                continue;
            }
            if (msg == MSGBASIC_MAGIC_TELEPORT && PTarget != PActionTarget)
            { // reset the no effect message above if somebody has gate crystal
                action.actionLists[0].actionTargets[0].messageID = MSGBASIC_NONE;
            }
        }
        actionTarget.messageID = msg;

        if (IsMagicCovered)
        {
            state.ApplyMagicCoverEnmity(POriginalTarget, PActionTarget, (CMobEntity*)this);
        }
        else
        {
            state.ApplyEnmity(PTarget, ce, ve);
        }

        if (PTarget->objtype == TYPE_MOB &&
            msg != MSGBASIC_SHADOW_ABSORB) // If message isn't the shadow loss message, because I had to move this outside of the above check for it.
        {
            luautils::OnMagicHit(this, PTarget, PSpell);
        }

        if (this == PTarget || // Casting on self or ally
            (this->PParty && PTarget->PParty &&
             ((this->PParty == PTarget->PParty) || (this->PParty->m_PAlliance && this->PParty->m_PAlliance == PTarget->PParty->m_PAlliance))))
        {
            if (PSpell->isHeal())
            {
                roeutils::event(ROE_HEALALLY, static_cast<CCharEntity*>(this), RoeDatagram("heal", actionTarget.param));

                // We know its an ally or self, if not self and leader matches, credit the RoE Objective
                if (this != PTarget && this->objtype == TYPE_PC && PTarget->objtype == TYPE_PC && static_cast<CCharEntity*>(this)->profile.unity_leader == static_cast<CCharEntity*>(PTarget)->profile.unity_leader)
                {
                    roeutils::event(ROE_HEAL_UNITYALLY, static_cast<CCharEntity*>(this), RoeDatagram("heal", actionTarget.param));
                }
            }
            else if (this != PTarget && PSpell->isBuff() && actionTarget.param)
            {
                roeutils::event(ROE_BUFFALLY, static_cast<CCharEntity*>(this), RoeDatagramList{});
            }
        }
    }
    if ((!(PSpell->isHeal()) || PSpell->tookEffect()) && PActionTarget->isAlive())
    {
        if (objtype != TYPE_PET)
        {
            battleutils::ClaimMob(PActionTarget, this);
        }
    }

    // TODO: Pixies will probably break here, once they're added.
    if (this->allegiance != PActionTarget->allegiance)
    {
        // Should not be removed by AoE effects that don't target the player or
        // buffs cast by other players or mobs.
        PActionTarget->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DETECTABLE);
    }

    this->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_MAGIC_END);
}

void CBattleEntity::OnCastInterrupted(CMagicState& state, action_t& action, MSGBASIC_ID msg, bool blockedCast)
{
    TracyZoneScoped;
    CSpell* PSpell = state.GetSpell();
    if (PSpell)
    {
        action.id         = id;
        action.spellgroup = PSpell->getSpellGroup();
        action.recast     = 0;
        action.actiontype = ACTION_MAGIC_INTERRUPT;

        actionList_t& actionList  = action.getNewActionList();
        actionList.ActionTargetID = id;

        actionTarget_t& actionTarget = actionList.getNewActionTarget();
        actionTarget.messageID       = 0;
        actionTarget.animation       = 0;
        actionTarget.param           = static_cast<uint16>(PSpell->getID());

        this->PAI->EventHandler.triggerListener("MAGIC_INTERRUPTED", CLuaBaseEntity(this));

        if (blockedCast)
        {
            // In a cutscene or otherwise, it seems the target "evades" the cast, despite the ID being set to the caster?
            // No message is sent in this case
            actionTarget.reaction = REACTION::EVADE;
        }
        else
        {
            actionTarget.reaction = REACTION::HIT;
            // For some reason, despite the system supporting interrupted message in the action packet (like auto attacks, JA), an 0x029 message is sent for spells.
            loc.zone->PushPacket(this, CHAR_INRANGE_SELF, new CMessageBasicPacket(this, state.GetTarget() ? state.GetTarget() : this, 0, 0, msg));
        }
    }
}

void CBattleEntity::OnWeaponSkillFinished(CWeaponSkillState& state, action_t& action)
{
    TracyZoneScoped;
    auto* PWeaponskill = state.GetSkill();

    action.id         = id;
    action.actiontype = ACTION_WEAPONSKILL_FINISH;
    action.actionid   = PWeaponskill->getID();
}

bool CBattleEntity::CanAttack(CBattleEntity* PTarget, std::unique_ptr<CBasicPacket>& errMsg)
{
    TracyZoneScoped;

    if (this->PMaster != nullptr && this->PMaster->objtype == TYPE_PC && !static_cast<CCharEntity*>(this->PMaster)->IsMobOwner(PTarget))
    {
        errMsg = std::make_unique<CMessageBasicPacket>(this, PTarget, 0, 0, MSGBASIC_ALREADY_CLAIMED);

        PAI->Disengage();
        return false;
    }

    if (PTarget->PAI->IsUntargetable())
    {
        return false;
    }

    auto distancecheck = (distance(loc.p, PTarget->loc.p) - PTarget->m_ModelRadius) > GetMeleeRange() || !PAI->GetController()->IsAutoAttackEnabled();

    if (this->objtype == TYPE_MOB &&
        this->GetHPP() < 100 &&
        !distancecheck &&
        (abs(this->loc.p.y - PTarget->loc.p.y) > 2 && abs(this->loc.p.y - PTarget->loc.p.y) < 10) &&
        !this->PAI->PathFind->IsFollowingPath())
    {
        this->PAI->PathFind->PathTo(PTarget->loc.p, PATHFLAG_WALLHACK);
    }
    return !distancecheck;
}

void CBattleEntity::OnDisengage(CAttackState& s)
{
    TracyZoneScoped;
    m_battleTarget = 0;
    if (animation == ANIMATION_ATTACK)
    {
        animation = ANIMATION_NONE;
    }
    updatemask |= UPDATE_HP;
    PAI->EventHandler.triggerListener("DISENGAGE", CLuaBaseEntity(this));
}

void CBattleEntity::OnChangeTarget(CBattleEntity* PTarget)
{
}

void CBattleEntity::setActionInterrupted(action_t& action, CBattleEntity* PTarget, uint16 messageID, uint16 actionID)
{
    if (PTarget)
    {
        actionList_t& actionList  = action.getNewActionList();
        actionList.ActionTargetID = PTarget->id;
        action.id                 = this->id;
        action.actiontype         = ACTION_MAGIC_FINISH; // all "is paralyzed" messages are cat 4
        action.actionid           = actionID;

        actionTarget_t& actionTarget = actionList.getNewActionTarget();
        actionTarget.animation       = 0x1FC; // Seems hardcoded, two bits away from 0x1FF (0x1FC = 1 1111 1100)
        actionTarget.messageID       = messageID;
        actionTarget.param           = 0;
    }
}

CBattleEntity* CBattleEntity::GetBattleTarget()
{
    return static_cast<CBattleEntity*>(GetEntity(GetBattleTargetID()));
}

bool CBattleEntity::OnAttack(CAttackState& state, action_t& action)
{
    TracyZoneScoped;
    auto* PTarget = static_cast<CBattleEntity*>(state.GetTarget());

    if (this->objtype == TYPE_MOB)
    {
        auto* PMob = dynamic_cast<CMobEntity*>(this);
        if (charutils::CheckMob(PTarget->m_mlvl, PMob->m_maxLevel) == EMobDifficulty::TooWeak)
        {
            PMob->m_ExpPenalty = ((PMob->m_ExpPenalty + lua["xi"]["settings"]["main"]["PL_PENALTY"].get<uint16>()) < UINT16_MAX - 1) ? PMob->m_ExpPenalty + lua["xi"]["settings"]["main"]["PL_PENALTY"].get<uint16>() : UINT16_MAX - 1;
        }
    }

    if (PTarget->objtype == TYPE_PC)
    {
        // TODO: Should not be removed by AoE effects that don't target the player.
        PTarget->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_DETECTABLE);
    }

    battleutils::ClaimMob(PTarget, this); // Mobs get claimed whether or not your attack actually is intimidated/paralyzed
    PTarget->LastAttacked = server_clock::now();

    if (battleutils::IsParalyzed(this))
    {
        setActionInterrupted(action, PTarget, MSGBASIC_IS_PARALYZED_2, 0);
        return true;
    }

    if (battleutils::IsIntimidated(this, PTarget))
    {
        setActionInterrupted(action, PTarget, MSGBASIC_IS_INTIMIDATED, 0);
        return true;
    }

    // Create a new attack round.
    CAttackRound attackRound(this, PTarget);

    action.actiontype  = ACTION_ATTACK;
    action.id          = this->id;
    actionList_t& list = action.getNewActionList();

    list.ActionTargetID = PTarget->id;

    CBattleEntity* POriginalTarget = PTarget;

    /////////////////////////////////////////////////////////////////////////
    //  Start of the attack loop.
    /////////////////////////////////////////////////////////////////////////
    while (attackRound.GetAttackSwingCount() && !(PTarget->isDead()))
    {
        actionTarget_t& actionTarget = list.getNewActionTarget();
        // Reference to the current swing.
        CAttack& attack = attackRound.GetCurrentAttack();

        // Set the swing animation.
        actionTarget.animation = attack.GetAnimationID();

        if (attack.CheckCover())
        {
            PTarget             = attackRound.GetCoverAbilityUserEntity();
            list.ActionTargetID = PTarget->id;
        }

        if (PTarget->StatusEffectContainer->HasStatusEffect(EFFECT_PERFECT_DODGE, 0))
        {
            actionTarget.messageID  = 32;
            actionTarget.reaction   = REACTION::EVADE;
            actionTarget.speceffect = SPECEFFECT::NONE;
        }
        else if ((xirand::GetRandomNumber(100) < attack.GetHitRate() || attackRound.GetSATAOccured()) &&
                 !PTarget->StatusEffectContainer->HasStatusEffect(EFFECT_ALL_MISS))
        {
            // attack hit, try to be deflected by parry unless it is a SATA attack round
            if (!(attackRound.GetSATAOccured()) && attack.IsParried())
            {
                actionTarget.messageID  = 70;
                actionTarget.reaction   = REACTION::PARRY | REACTION::HIT;
                actionTarget.speceffect = SPECEFFECT::NONE;
                battleutils::HandleTacticalParry(PTarget);
                battleutils::HandleIssekiganEnmityBonus(PTarget, this);
            }
            // attack hit, try to be absorbed by shadow unless it is a SATA attack round
            else if (!(attackRound.GetSATAOccured()) && battleutils::IsAbsorbByShadow(PTarget, this))
            {
                actionTarget.messageID = MSGBASIC_SHADOW_ABSORB;
                actionTarget.param     = 1;
                actionTarget.reaction  = REACTION::EVADE;
                attack.SetEvaded(true);
            }
            else if (attack.CheckAnticipated() || attack.CheckCounter())
            {
                if (attack.IsAnticipated())
                {
                    actionTarget.messageID  = 30;
                    actionTarget.reaction   = REACTION::EVADE;
                    actionTarget.speceffect = SPECEFFECT::NONE;
                }
                if (attack.IsCountered())
                {
                    actionTarget.reaction     = REACTION::EVADE;
                    actionTarget.speceffect   = SPECEFFECT::NONE;
                    actionTarget.param        = 0;
                    actionTarget.messageID    = 0;
                    actionTarget.spikesEffect = SUBEFFECT_COUNTER;
                    if (battleutils::IsAbsorbByShadow(this, PTarget))
                    {
                        actionTarget.spikesParam   = 1;
                        actionTarget.spikesMessage = MSGBASIC_COUNTER_ABS_BY_SHADOW;
                        actionTarget.messageID     = 0;
                        actionTarget.param         = 0;
                    }
                    else
                    {
                        int16 naturalh2hDMG = 0;
                        if (auto* targ_weapon = dynamic_cast<CItemWeapon*>(PTarget->m_Weapons[SLOT_MAIN]);
                            (targ_weapon && targ_weapon->getSkillType() == SKILL_HAND_TO_HAND) ||
                            (PTarget->objtype == TYPE_MOB && PTarget->GetMJob() == JOB_MNK))
                        {
                            naturalh2hDMG = (int16)((PTarget->GetSkill(SKILL_HAND_TO_HAND) * 0.11f) + 3);
                        }

                        // Calculate attack bonus for Counterstance Effect Job Points
                        // Needs verification, as there appears to be conflicting information regarding an attack bonus based on DEX
                        // vs a base damage increase.
                        float csJpAtkBonus = 1;
                        if (PTarget->objtype == TYPE_PC && PTarget->GetMJob() == JOB_MNK && PTarget->StatusEffectContainer->HasStatusEffect(EFFECT_COUNTERSTANCE))
                        {
                            auto*  PChar        = static_cast<CCharEntity*>(PTarget);
                            uint8  csJpModifier = PChar->PJobPoints->GetJobPointValue(JP_COUNTERSTANCE_EFFECT) * 2;
                            uint16 targetDex    = PTarget->DEX();

                            csJpAtkBonus = 1 + ((static_cast<float>(targetDex) / 100) * csJpModifier);
                        }

                        float DamageRatio = battleutils::GetDamageRatio(PTarget, this, attack.IsCritical(), csJpAtkBonus, SLOT_MAIN, 0, false);
                        auto  damage      = 0;

                        if (PTarget->objtype == TYPE_MOB)
                        {
                            int8 mobH2HDamage = PTarget->GetMLevel() + 2;
                            damage            = (int32)((std::floor((mobH2HDamage + battleutils::GetFSTR(PTarget, this, SLOT_MAIN)) * 0.9f) / 2) * DamageRatio);
                        }
                        else
                        {
                            damage = (int32)((PTarget->GetMainWeaponDmg() + naturalh2hDMG + battleutils::GetFSTR(PTarget, this, SLOT_MAIN)) * DamageRatio);
                        }

                        actionTarget.spikesParam =
                            battleutils::TakePhysicalDamage(PTarget, this, attack.GetAttackType(), damage, false, SLOT_MAIN, 1, nullptr, true, false, true);
                        actionTarget.spikesMessage = 33;
                        if (PTarget->objtype == TYPE_PC)
                        {
                            auto* targ_weapon = dynamic_cast<CItemWeapon*>(PTarget->m_Weapons[SLOT_MAIN]);
                            uint8 skilltype   = (targ_weapon == nullptr ? (uint8)SKILL_HAND_TO_HAND : targ_weapon->getSkillType());
                            charutils::TrySkillUP((CCharEntity*)PTarget, (SKILLTYPE)skilltype, GetMLevel());
                        } // In case the Automaton can counter
                        else if (PTarget->objtype == TYPE_PET && PTarget->PMaster && PTarget->PMaster->objtype == TYPE_PC &&
                                 static_cast<CPetEntity*>(PTarget)->getPetType() == PET_TYPE::AUTOMATON)
                        {
                            puppetutils::TrySkillUP((CAutomatonEntity*)PTarget, SKILL_AUTOMATON_MELEE, GetMLevel());
                        }
                    }
                }
            }
            else
            {
                // Set this attack's critical flag.
                SLOTTYPE weaponSlot = (SLOTTYPE)attack.GetWeaponSlot();
                attack.SetCritical(xirand::GetRandomNumber(100) < battleutils::GetCritHitRate(this, PTarget, !attack.IsFirstSwing(), weaponSlot), weaponSlot, attack.IsGuarded());

                actionTarget.reaction = REACTION::HIT;

                if (this->objtype == TYPE_PC && weaponSlot == SLOT_MAIN && ((CCharEntity*)this)->getCharVar("TREDECIM_COUNTER") > 12)
                {
                    // Check for Tredecim
                    auto* equippedWeapon = dynamic_cast<CItemWeapon*>(this->m_Weapons[SLOT_MAIN]);
                    if (equippedWeapon->getID() == 18052)
                    {
                        // Ensure critical is only set to main weapon slot
                        attack.SetCritical(true, weaponSlot, attack.IsGuarded());
                        ((CCharEntity*)this)->setCharVar("TREDECIM_COUNTER", -1);
                    }
                }

                // Critical hit.
                if (attack.IsCritical())
                {
                    actionTarget.speceffect = SPECEFFECT::CRITICAL_HIT;
                    actionTarget.messageID  = attack.GetAttackType() == PHYSICAL_ATTACK_TYPE::DAKEN ? 353 : 67;

                    if (PTarget->objtype == TYPE_MOB)
                    {
                        // Listener (hook)
                        PTarget->PAI->EventHandler.triggerListener("CRITICAL_TAKE", CLuaBaseEntity(PTarget), CLuaBaseEntity(this));

                        // Binding
                        luautils::OnCriticalHit(PTarget, this);
                    }
                }
                // Not critical hit.
                else
                {
                    actionTarget.speceffect = SPECEFFECT::HIT;
                    actionTarget.messageID  = attack.GetAttackType() == PHYSICAL_ATTACK_TYPE::DAKEN ? 352 : 1;
                }

                // Guarded. TODO: Stuff guards that shouldn't.
                if (attack.IsGuarded())
                {
                    actionTarget.reaction |= REACTION::GUARDED;
                    battleutils::HandleTacticalGuard(PTarget);
                }

                // Apply Feint
                if (CStatusEffect* PFeintEffect = StatusEffectContainer->GetStatusEffect(EFFECT_FEINT))
                {
                    PTarget->StatusEffectContainer->AddStatusEffect(
                        new CStatusEffect(EFFECT_EVASION_DOWN, EFFECT_EVASION_DOWN, PFeintEffect->GetPower(), 3, 30));
                    StatusEffectContainer->DelStatusEffect(EFFECT_FEINT);
                }

                // Process damage.
                attack.ProcessDamage(attack.IsCritical(), attack.IsGuarded(), attack.GetAttackType() == PHYSICAL_ATTACK_TYPE::KICK);

                // Try shield block
                if (attack.IsBlocked())
                {
                    actionTarget.reaction |= REACTION::BLOCK;
                }

                actionTarget.param =
                    battleutils::TakePhysicalDamage(this, PTarget, attack.GetAttackType(), attack.GetDamage(), attack.IsBlocked(), weaponSlot, 1,
                                                    attackRound.GetTAEntity(), true, true, attack.IsCountered(), attack.IsCovered(), POriginalTarget);
                if (actionTarget.param < 0)
                {
                    actionTarget.param     = -(actionTarget.param);
                    actionTarget.messageID = 373;
                }
            }

            if (PTarget->objtype == TYPE_PC)
            {
                if (attack.IsGuarded(false) || !settings::get<bool>("map.GUARD_OLD_SKILLUP_STYLE"))
                {
                    if (battleutils::GetGuardRate(this, PTarget) > 0)
                    {
                        charutils::TrySkillUP((CCharEntity*)PTarget, SKILL_GUARD, GetMLevel());
                    }
                }

                if (attack.IsBlocked() || !settings::get<bool>("map.BLOCK_OLD_SKILLUP_STYLE"))
                {
                    if (battleutils::GetBlockRate(this, PTarget) > 0)
                    {
                        charutils::TrySkillUP((CCharEntity*)PTarget, SKILL_SHIELD, GetMLevel());
                    }
                }

                if (attack.IsParried(false) || !settings::get<bool>("map.PARRY_OLD_SKILLUP_STYLE"))
                {
                    if (battleutils::GetParryRate(this, PTarget) > 0)
                    {
                        charutils::TrySkillUP((CCharEntity*)PTarget, SKILL_PARRY, GetMLevel());
                    }
                }

                if (!attack.IsCountered() && !attack.IsParried(false))
                {
                    charutils::TrySkillUP((CCharEntity*)PTarget, SKILL_EVASION, GetMLevel());
                }
            }
        }
        else
        {
            // misses the target
            actionTarget.reaction   = REACTION::EVADE;
            actionTarget.speceffect = SPECEFFECT::NONE;
            actionTarget.messageID  = 15;
            attack.SetEvaded(true);

            // Check & Handle Afflatus Misery Accuracy Bonus
            battleutils::HandleAfflatusMiseryAccuracyBonus(this);

            if (PTarget->objtype == TYPE_PC)
            {
                charutils::TrySkillUP((CCharEntity*)PTarget, SKILL_EVASION, GetMLevel());
            }

            if (PTarget->objtype == TYPE_MOB && this->objtype == TYPE_PC)
            {
                // 1 ce for a missed attack for TH application
                ((CMobEntity*)PTarget)->PEnmityContainer->UpdateEnmity(this, 1, 0);
            }
        }

        // If we didn't hit at all, set param to 0 if we didn't blink any shadows.
        if ((actionTarget.reaction & REACTION::HIT) == REACTION::NONE && actionTarget.messageID != MSGBASIC_SHADOW_ABSORB)
        {
            actionTarget.param = 0;
        }

        // if we did hit, run enspell/spike routines as long as this isn't a Daken swing
        if ((actionTarget.reaction & REACTION::MISS) == REACTION::NONE && attack.GetAttackType() != PHYSICAL_ATTACK_TYPE::DAKEN)
        {
            battleutils::HandleEnspell(this, PTarget, &actionTarget, attack.IsFirstSwing(), (CItemWeapon*)this->m_Weapons[attack.GetWeaponSlot()],
                                       attack.GetDamage());
            battleutils::HandleSpikesDamage(this, PTarget, &actionTarget, attack.GetDamage());
        }

        // if we parried, run battuta check if applicable
        if ((actionTarget.reaction & REACTION::PARRY) == REACTION::PARRY && PTarget->StatusEffectContainer->HasStatusEffect(EFFECT_BATTUTA))
        {
            battleutils::HandleParrySpikesDamage(this, PTarget, &actionTarget, attack.GetDamage());
        }

        // set recoil if we hit and dealt non-zero damage
        if (actionTarget.speceffect == SPECEFFECT::HIT && actionTarget.param > 0)
        {
            actionTarget.speceffect = SPECEFFECT::RECOIL;
        }

        // try zanshin only on single swing attack rounds - it is last priority in the multi-hit order
        // if zanshin procs, add a new zanshin based attack.
        if (attack.IsFirstSwing() && attackRound.GetAttackSwingCount() == 1)
        {
            uint16 zanshinChance = this->getMod(Mod::ZANSHIN) + battleutils::GetMeritValue(this, MERIT_ZASHIN_ATTACK_RATE);
            zanshinChance        = std::clamp<uint16>(zanshinChance, 0, 100);
            // zanshin may only proc on a missed/guarded/countered swing or as SAM main with hasso up (at 25% of the base zanshin rate)
            if ((((actionTarget.reaction & REACTION::MISS) != REACTION::NONE || (actionTarget.reaction & REACTION::GUARDED) != REACTION::NONE || actionTarget.spikesEffect == SUBEFFECT_COUNTER) &&
                 xirand::GetRandomNumber(100) < zanshinChance))
            {
                attackRound.AddAttackSwing(PHYSICAL_ATTACK_TYPE::ZANSHIN, PHYSICAL_ATTACK_DIRECTION::RIGHTATTACK, 1);
            }
        }

        attackRound.DeleteAttackSwing();

        if (list.actionTargets.size() == 8)
        {
            break;
        }
    }

    PAI->EventHandler.triggerListener("ATTACK", CLuaBaseEntity(this), CLuaBaseEntity(PTarget), CLuaAction(&action));
    PTarget->PAI->EventHandler.triggerListener("ATTACKED", CLuaBaseEntity(PTarget), CLuaBaseEntity(this), CLuaAction(&action));
    /////////////////////////////////////////////////////////////////////////////////////////////
    // End of attack loop
    /////////////////////////////////////////////////////////////////////////////////////////////

    this->StatusEffectContainer->DelStatusEffectsByFlag(EFFECTFLAG_ATTACK | EFFECTFLAG_DETECTABLE);

    return true;
}

CBattleEntity* CBattleEntity::IsValidTarget(uint16 targid, uint16 validTargetFlags, std::unique_ptr<CBasicPacket>& errMsg)
{
    TracyZoneScoped;
    auto* PTarget = PAI->TargetFind->getValidTarget(targid, validTargetFlags);
    return PTarget;
}

void CBattleEntity::OnEngage(CAttackState& state)
{
    TracyZoneScoped;
    animation = ANIMATION_ATTACK;
    updatemask |= UPDATE_HP;
    PAI->EventHandler.triggerListener("ENGAGE", CLuaBaseEntity(this), CLuaBaseEntity(state.GetTarget()));
}

void CBattleEntity::TryHitInterrupt(CBattleEntity* PAttacker)
{
    if (PAI->GetCurrentState())
    {
        PAI->GetCurrentState()->TryInterrupt(PAttacker);
    }
}

void CBattleEntity::OnDespawn(CDespawnState& /*unused*/)
{
    TracyZoneScoped;
    FadeOut();
    // #event despawn
    PAI->EventHandler.triggerListener("DESPAWN", CLuaBaseEntity(this));
    PAI->Internal_Respawn(0s);
}

void CBattleEntity::SetBattleStartTime(time_point time)
{
    m_battleStartTime = time;
}

duration CBattleEntity::GetBattleTime()
{
    return server_clock::now() - m_battleStartTime;
}

void CBattleEntity::setBattleID(uint16 battleID)
{
    m_battleID = battleID;
}

uint16 CBattleEntity::getBattleID()
{
    return m_battleID;
}

void CBattleEntity::Tick(time_point /*unused*/)
{
    TracyZoneScoped;
}

void CBattleEntity::PostTick()
{
    TracyZoneScoped;
    if (health.hp == 0 && PAI->IsSpawned() && !PAI->IsCurrentState<CDeathState>() && !PAI->IsCurrentState<CRaiseState>() &&
        !PAI->IsCurrentState<CDespawnState>())
    {
        Die();
    }
}

uint16 CBattleEntity::GetBattleTargetID() const
{
    return m_battleTarget;
}
