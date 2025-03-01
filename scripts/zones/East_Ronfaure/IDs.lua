-----------------------------------
-- Area: East_Ronfaure
-----------------------------------
require("scripts/globals/zone")
require("scripts/globals/keyitems")
-----------------------------------

zones = zones or {}

zones[xi.zone.EAST_RONFAURE] =
{
    text =
    {
        NOTHING_HAPPENS               = 141,   -- Nothing happens...
        ITEM_CANNOT_BE_OBTAINED       = 6406,  -- You cannot obtain the <item>. Come back after sorting your inventory.
        ITEM_OBTAINED                 = 6412,  -- Obtained: <item>.
        GIL_OBTAINED                  = 6413,  -- Obtained <number> gil.
        KEYITEM_OBTAINED              = 6415,  -- Obtained key item: <keyitem>.
        NOTHING_OUT_OF_ORDINARY       = 6426,  -- There is nothing out of the ordinary here.
        FELLOW_MESSAGE_OFFSET         = 6441,  -- I'm ready. I suppose.
        CARRIED_OVER_POINTS           = 7023,  -- You have carried over <number> login point[/s].
        LOGIN_CAMPAIGN_UNDERWAY       = 7024,  -- The [/January/February/March/April/May/June/July/August/September/October/November/December] <number> Login Campaign is currently underway!
        LOGIN_NUMBER                  = 7025,  -- In celebration of your most recent login (login no. <number>), we have provided you with <number> points! You currently have a total of <number> points.
        MEMBERS_LEVELS_ARE_RESTRICTED = 7045,  -- Your party is unable to participate because certain members' levels are restricted.
        CONQUEST_BASE                 = 7082,  -- Tallying conquest results...
        FISHING_MESSAGE_OFFSET        = 7241,  -- You can't fish here.
        DIG_THROW_AWAY                = 7254,  -- You dig up <item>, but your inventory is full. You regretfully throw the <item> away.
        FIND_NOTHING                  = 7256,  -- You dig and you dig, but find nothing.
        RAYOCHINDOT_DIALOG            = 7421,  -- If you are outmatched, run to the city as quickly as you can.
        CROTEILLARD_DIALOG            = 7422,  -- Sorry, no chatting while I'm on duty.
        NAME_IS_ANDELAIN              = 7423,  -- My name is Andelain. As part of my devotions, I come here each day to pray.
        NO_MORE_NO_LESS               = 7424,  -- During this time, I may eat only three <ITEM_ID> a day for nourishment. No more, no less. And no other food may I eat.
        THANKS_TO_GODDESS             = 7425,  -- Thanks be to the Goddess in her benevolence!
        CANNOT_ACCEPT_THIS            = 7426,  -- I am currently undergoing devotions, and as such, am not allowed to take alms from those on the road. I am sorry, but I cannot accept this
        GATES_OF_PARADISE             = 7427,  -- May the Gates of Paradise open to all....
        ONLY_NEED_ONE                 = 7428,  -- I appreciate your offer, but I only need one <ITEM_ID>. Thank you for your kindness.
        CHEVAL_RIVER_WATER            = 7447,  -- You fill your waterskin with water from the river. You now have <item>.
        BLESSED_WATERSKIN             = 7466,  -- To get water, trade the waterskin you hold with the river.
        LOGGING_IS_POSSIBLE_HERE      = 7497,  -- Logging is possible here if you have <item>.
        PLAYER_OBTAINS_ITEM           = 7508,  -- <name> obtains <item>!
        UNABLE_TO_OBTAIN_ITEM         = 7509,  -- You were unable to obtain the item.
        PLAYER_OBTAINS_TEMP_ITEM      = 7510,  -- <name> obtains the temporary item: <item>!
        ALREADY_POSSESS_TEMP          = 7511,  -- You already possess that temporary item.
        NO_COMBINATION                = 7516,  -- You were unable to enter a combination.
        VOIDWALKER_DESPAWN            = 7547,  -- The monster fades before your eyes, a look of disappointment on its face.
        UNITY_WANTED_BATTLE_INTERACT  = 7578,  -- Those who have accepted % must pay # Unity accolades to participate. The content for this Wanted battle is #. [Ready to begin?/You do not have the appropriate object set, so your rewards will be limited.]
        REGIME_REGISTERED             = 9888,  -- New training regime registered!
        VOIDWALKER_NO_MOB             = 11061, -- The <keyitem> quivers ever so slightly, but emits no light. There seem to be no monsters in the area.
        VOIDWALKER_MOB_TOO_FAR        = 11062, -- The <keyitem> quivers ever so slightly and emits a faint light. There seem to be no monsters in the immediate vicinity.
        VOIDWALKER_MOB_HINT           = 11063, -- The <keyitem> resonates [feebly/softly/solidly/strongly/very strongly/furiously], sending a radiant beam of light lancing towards a spot roughly <number> [yalm/yalms] [east/southeast/south/southwest/west/northwest/north/northeast] of here.
        VOIDWALKER_SPAWN_MOB          = 11064, -- A monster materializes out of nowhere!
        VOIDWALKER_UPGRADE_KI_1       = 11066, -- The <keyitem> takes on a slightly deeper hue and becomes <keyitem>!
        VOIDWALKER_UPGRADE_KI_2       = 11067, -- The <keyitem> takes on a deeper, richer hue and becomes <keyitem>!
        VOIDWALKER_BREAK_KI           = 11068, -- The <keyitem> shatters into tiny fragments.
        VOIDWALKER_OBTAIN_KI          = 11069, -- Obtained key item: <keyitem>!
        LEARNS_SPELL                  = 11959, -- <name> learns <spell>!
        UNCANNY_SENSATION             = 11961, -- You are assaulted by an uncanny sensation.
        SMILEBRINGER_START            = 11031, -- So, are you ready-steady? Beginner level training...GO! Come on, pump those legs like there's no tomorrow!
        SMILEHELPER_IDLE              = 11059, -- This is a checkpointaru for the Smilebringer Boot camp. To learny-wearn more, please speak with the smile sergeant posted by the town entrance.
        SMILEHELPER_CHECKPOINT_1      = 11053, -- Checkpoint <number> cleared, Elasped time: [minutes], [seconds]
        SMILEHELPER_CHECKPOINT_2      = 11054, -- Checkpoint <number> cleared! You gain the effect of Flee. Elasped time: [minutes], [seconds]
        SMILEHELPER_ALREADY_VISITED   = 11055, -- You have already cleared this checkpoint. Elapsed time: [minutes], [seconds]
        SMILEHELPER_POINTS_CLEARED    = 11057, -- All checkpoints cleared! Please report back to the smile sergeant in charge.
    },

    mob =
    {
        BIGMOUTH_BILLY_PH =
        {
            [17191194] = 17191196, -- 453.625 -18.436 -127.048
            [17191195] = 17191196, -- 403.967 -36.822 -16.285
        },

        SWAMFISK_PH =
        {
            [17191187] = 17191189, -- 379.120 -27.898 -46.436
            [17191188] = 17191189, -- 417.542 -17.210 -177.883
            [17191287] = 17191291, -- 443.334 -17.000 -303.275
            [17191288] = 17191291, -- 423.000 -16.000 -285.000
            [17191289] = 17191291, -- 461.268 -6.674 -391.342
            [17191290] = 17191291, -- 459.345 -6.686 -363.842
        },

        VOIDWALKER =
        {
            [xi.keyItem.CLEAR_ABYSSITE] =
            {
                17191334, -- Sunderclaw
                17191333, -- Sunderclaw
                17191332, -- Sunderclaw
                17191331, -- Sunderclaw
                17191330,  -- Quagmire Pugil
                17191329,  -- Quagmire Pugil
                17191328,  -- Quagmire Pugil
                17191327,  -- Quagmire Pugil
            },

            [xi.keyItem.COLORFUL_ABYSSITE] =
            {
                17191326, -- Capricornus
                17191325  -- Yacumama
            },

            [xi.keyItem.BLUE_ABYSSITE] =
            {
                17191324  -- Krabkatoa
            },

            [xi.keyItem.BLACK_ABYSSITE] =
            {
                17191323  -- Yilbegan
            }
        },
        RAMBUKK = 17191044,
    },

    npc =
    {
        LOGGING =
        {
            17191530,
            17191531,
            17191532,
            17191533,
            17191534,
            17191535,
        },
        STARLIGHT_DECORATIONS =
        {
            [17191562] = 17191562,  -- Stampeding Bison (Smilebringer Bootcamp Sergeant)
        },
        SMILE_HELPERS =
        {
            [17191563] = 17191563,  -- Smile Helper
            [17191564] = 17191564,  -- Smile Helper
            [17191565] = 17191565,  -- Smile Helper
            [17191566] = 17191566,  -- Smile Helper
            [17191567] = 17191567,  -- Smile Helper
            [17191568] = 17191568,  -- Smile Helper
            [17191569] = 17191569,  -- Smile Helper
            [17191570] = 17191570,  -- Smile Helper
            [17191571] = 17191571,  -- Smile Helper
            [17191572] = 17191572,  -- Smile Helper
        },
    },
}

return zones[xi.zone.EAST_RONFAURE]
