-----------------------------------
-- Area: Abyssea-Konschtat
-----------------------------------
require("scripts/globals/keyitems")
require("scripts/globals/zone")
-----------------------------------

zones = zones or {}

zones[xi.zone.ABYSSEA_KONSCHTAT] =
{
    text =
    {
        ITEM_CANNOT_BE_OBTAINED       = 6384, -- You cannot obtain the <item>. Come back after sorting your inventory.
        ITEM_OBTAINED                 = 6390, -- Obtained: <item>.
        GIL_OBTAINED                  = 6391, -- Obtained <number> gil.
        KEYITEM_OBTAINED              = 6393, -- Obtained key item: <keyitem>.
        LOST_KEYITEM                  = 6394, -- Lost key item: <keyitem>.
        CRUOR_TOTAL                   = 6988, -- Obtained <number> cruor. (Total: <number>)
        CARRIED_OVER_POINTS           = 7001, -- You have carried over <number> login point[/s].
        LOGIN_CAMPAIGN_UNDERWAY       = 7002, -- The [/January/February/March/April/May/June/July/August/September/October/November/December] <number> Login Campaign is currently underway!
        LOGIN_NUMBER                  = 7003, -- In celebration of your most recent login (login no. <number>), we have provided you with <number> points! You currently have a total of <number> points.
        MEMBERS_LEVELS_ARE_RESTRICTED = 7023, -- Your party is unable to participate because certain members' levels are restricted.
        LIGHTS_MESSAGE_1              = 7325, -- Visitant Light Intensity Pearlescent: <number> / Ebon: <number> Golden: <number> / Silvery: <number>
        LIGHTS_MESSAGE_2              = 7326, -- Azure: <number> / Ruby: <number> / Amber: <number>
        STAGGERED                     = 7327, -- <name>'s attack staggers the fiend!
        YELLOW_STAGGER                = 7328, -- The fiend is unable to cast magic.
        BLUE_STAGGER                  = 7329, -- The fiend is unable to use special attacks.
        RED_STAGGER                   = 7330, -- The fiend is frozen in its tracks.
        YELLOW_WEAKNESS               = 7331, -- The fiend appears vulnerable to [/fire/ice/wind/earth/lightning/water/light/darkness] elemental magic!
        BLUE_WEAKNESS                 = 7332, -- The fiend appears vulnerable to [/hand-to-hand/dagger/sword/great sword/axe/great axe/scythe/polearm/katana/great katana/club/staff/archery/marksmanship] weapon skills!
        RED_WEAKNESS                  = 7333, -- The fiend appears vulnerable to [/fire/ice/wind/earth/lightning/water/light/darkness] elemental weapon skills!
        ABYSSEA_TIME_OFFSET           = 7334, -- Your visitant status will wear off in <number> [second/minute].
        RETURNING_TO_SEARING_IN       = 7343, -- Returning to the Searing Ward in <number> [second/seconds].
        NO_VISITANT_WARD              = 7344, -- You do not have visitant status. Returning to the Searing Ward in <number> [second/seconds].
        RETURNING_TO_WARD             = 7346, -- Returning to the Searing Ward now.
        ALREADY_POSSESS_KEY_ITEM      = 7402, -- It appears you already possess that key item.
        NO_VISITANT_STATUS            = 7403, -- You do not have visitant status. Please proceed to the nearest Conflux Surveyor to have it granted.
        ATMA_INFUSED                  = 7446, -- <name> expends <number> cruor and is now infused with <atma>!
        ATMA_PURGED                   = 7447, -- <name> has been purged of the <atma>.
        ALL_ATMA_PURGED               = 7448, -- <name> has been purged of all infused atma.
        PREVIOUS_ATMA_INFUSED         = 7454, -- <name> expends <number> cruor and his/her previous atma configuration is restored!
        HISTORY_ATMA_INFUSED          = 7461, -- <name> expends <number> cruor and is now infused with his/her chosen atma set!
        MONSTER_CONCEALED_CHEST       = 7486, -- The monster was concealing a treasure chest!
        OBTAINS_TEMP_ITEM             = 7496, -- <name> obtains the temporary item: <item>!
        OBTAINS_ITEM                  = 7497, -- <name> obtains the item: <item>!
        OBTAINS_KEYITEM               = 7498, -- <name> obtains the key item: <item>!
        ADD_SPOILS_TO_TREASURE        = 7499, -- <name> transferred the contents of the pyxis to the cache of lottable spoils.
        TEMP_ITEM_DISAPPEARED         = 7502, -- That temporary item had already disappeared.
        KEYITEM_DISAPPEARED           = 7503, -- That key item had already disappeared.
        ITEM_DISAPPEARED              = 7504, -- That item had already disappeared.
        CHEST_DESPAWNED               = 7505, -- The treasure chest had already disappeared.
        CRUOR_OBTAINED                = 7506, -- <name> obtained <number> cruor.
        OBTAINS_SEVERAL_TEMPS         = 7507, -- <name> obtains several temporary items!
        BODY_EMITS_OFFSET             = 7508, -- <name>'s body emits [a faint/a mild/a strong] pearlescent light!
        CANNOT_OPEN_CHEST             = 7515, -- You cannot open that treasure chest.
        PLAYER_HAS_CLAIM_OF_CHEST     = 7516, -- <name> has claim over that treasure chest.
        PARTY_NOT_OWN_CHEST           = 7517, -- Your party does not have claim over that treasure chest.
        CHEST_DISAPPEARED             = 7520, -- The treasure chest has disappeared.
        RANDOM_SUCCESS_FAIL_GUESS     = 7542, -- The randomly generated number was <number>! <name> guessed [successfully/unsuccessfully]!
        AIR_PRESSURE_CHANGE           = 7546, -- <name> [reduced/increased] the air pressure by <number> units. Current air pressure: <number>[/ (minimum)/ (maximum)]
        INPUT_ERROR                   = 7550, -- Please input a two-digit combination between <number><number>.
        INPUT_SUCCESS_FAIL_GUESS      = 7551, -- <name> inputs the number <number>[, but nothing happens./, successfully unlocking the chest!]
        GREATER_OR_LESS_THAN          = 7552, -- You have a hunch that the lock's combination is [greater/less] than <number>.
        HUNCH_SECOND_FIRST_EVEN_ODD   = 7553, -- You have a hunch that the [second/first] digit is [even/odd].
        HUNCH_SECOND_FIRST_IS         = 7554, -- You have a hunch that the [second/first] digit is <number>.
        HUNCH_SECOND_FIRST_IS_OR      = 7555, -- You have a hunch that the [second/first] digit is <number>, <number>, or <number>.
        HUNCH_ONE_DIGIT_IS            = 7556, -- You have a hunch that one of the digits is <number>.
        HUNCH_SUM_EQUALS              = 7557, -- You have a hunch that the sum of the two digits is <number>.
        PLAYER_OPENED_LOCK            = 7558, -- <name> succeeded in opening the lock!
        PLAYER_FAILED_LOCK            = 7559, -- <name> failed to open the lock.
        TRADE_KEY_OPEN                = 7560, -- <name> uses <item> and opens the lock!
        BOUNDLESS_RAGE                = 7583, -- You sense an aura of boundless rage...
        INFO_KI                       = 7584, -- Your keen senses tell you that something may happen if only you had [this item/these items].
        USE_KI                        = 7587, -- Use the [key item/key items]? Yes. No.
    },
    mob =
    {
        GUIMAUVE                = 16838912,
        ASHTAERH_THE_GALLVEXED  = 16838718,
        SARCOPHILUS             = 16838767,
        BOMBADEEL               = 16838782,
        HEXENPILZ               = 16838837,
        KERATYRANNOS            = 16838871,
        LENTOR                  = 16838885,
        BLOODGUZZLER            = 16838899,
        CLINGY_CLARE            = 16838946,
        SIRANPA_KAMUY           = 16838962,
        ALKONOST                = 16838979,
        ARIMASPI                = 16838993,
        FEAR_GORTA              = 16839033,
        KUKULKAN_1              = 16838872,
        ECCENTRIC_EVE_1         = 16839007,
        BLOODEYE_VILEBERRY_1    = 16839068,
        ECCENTRIC_EVE_2         = 16839069,
        KUKULKAN_2              = 16839070,
        BLOODEYE_VILEBERRY_2    = 16839071,
        ECCENTRIC_EVE_3         = 16839072,
        KUKULKAN_3              = 16839073,
        BLOODEYE_VILEBERRY_3    = 16839074,
    },
    npc =
    {
    },
}

return zones[xi.zone.ABYSSEA_KONSCHTAT]
