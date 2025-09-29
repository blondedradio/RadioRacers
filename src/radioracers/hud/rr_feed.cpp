// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_feed.cpp
/// \brief Hardcode port of killfeed Lua script from ~2018
/// \brief https://github.com/blondedradio/RadioRacers/issues/21

#include <algorithm>
#include <array>
#include <vector>
#include <deque>
#include <string>
#include <utility>

#include <iostream>
#include <memory>

#include "rr_feed.hpp"
#include "../rr_hud.h"
#include "../rr_setup.h"
#include "../rr_util.h"
#include "../rr_cvar.h"
#include "../../i_time.h"
#include "../../k_rank.h"
#include "../../doomtype.h"
#include "../../p_local.h"
#include "../../p_tick.h"
#include "../../m_easing.h"
#include "../../info.h"
#include "../../v_video.h"
#include "../../command.h"
#include "../../m_random.h"
#include "../../k_objects.h"
#include "../../v_draw.hpp" // srb2:Draw

using srb2::Draw;
const BaseFeedUpdate::PatchManager BaseFeedUpdate::patches_;
static boolean has_feed_been_initialized = false;

// === CLASSES ===

// 1. Player interaction (hit/explode/whip etc)
class PlayerFeedUpdate : public BaseFeedUpdate {
    private:
        std::string victim {"Victim"};          // Target's name
        std::string attacker {"Attacker"};      // Source's name
        ItemConfigForFeedUpdate item {};        // The item to blame
        boolean is_self_hit {false};            // Self-hit (how emabrassing)

        // Drawer
        Draw row;
        ProcessedItemConfig c;

        void draw_attacker(boolean reverse) {
            int attacker_name_width = FeedUpdateUtils::getNameWidth(attacker);
            if (reverse) {
                if (!is_self_hit) {
                    row
                        .x(-attacker_name_width)
                        .scale(FEED_UPDATE_SCALE)
                        .text(attacker.c_str());
                }
            } else {                
                UINT8 *item_recolormap = (c.should_flash) ? c.colormap : c.item_recolormap;
                if (!is_self_hit) {
                    row.scale(FEED_UPDATE_SCALE).text(attacker.c_str());
                    
                    // Then, the item
                    row = row.x(attacker_name_width + c.x_offset).colormap(item_recolormap);
                } else {
                    // Begin the update with the item
                    row = row.colormap(item_recolormap);
                }
            }
        }

        void draw_item(boolean reverse) {
            int x = 0;
            if (reverse) {
                x = -c.width_scaled;
            }

            row
                .x(x)
                .y(c.y_offset)
                .scale(c.patch_scale)
                .patch(c.patch_name);

            if (reverse) {
                row = row.x(-(c.width_scaled + c.x_offset)).colormap(static_cast<UINT8*>(NULL));
            } else {
                row = row.x(c.width_scaled + c.x_offset).colormap(static_cast<UINT8*>(NULL));
            }
        }

        void draw_victim(boolean reverse) {
            if (reverse) {
                int victim_name_width = FeedUpdateUtils::getNameWidth(victim);
                row.x(-(victim_name_width)).scale(FEED_UPDATE_SCALE).text(victim.c_str());
                row = row.x(-(victim_name_width + c.x_offset)).colormap(c.colormap);
            } else {
                row.scale(FEED_UPDATE_SCALE).text(victim.c_str());
            }
        }

        int get_background_width() {
            int attacker_name_width = FeedUpdateUtils::getNameWidth(attacker);
            int victim_name_width = FeedUpdateUtils::getNameWidth(victim);

            int background_width = 0;

            if (!is_self_hit) {
                background_width += (attacker_name_width + c.x_offset);
            }

            background_width += (c.width_scaled + c.x_offset) + (victim_name_width - 2);
            return background_width;
        }

        // Get the width of the update up until the item graphic
        int get_width_for_centering() {
            int attacker_name_width = FeedUpdateUtils::getNameWidth(attacker);
            int center_width = 0;

            if (!is_self_hit) {
                center_width += (attacker_name_width + c.x_offset);
            }
            center_width += (c.width_scaled / 2);
            return center_width;
        }
        
    public:
        // https://www.geeksforgeeks.org/when-do-we-use-initializer-list-in-c/
        PlayerFeedUpdate(
            const std::string &v, 
            const std::string &a,
            const ItemConfigForFeedUpdate &i,
            const boolean &self_hit,
            const boolean &about_self
        ): victim(v), attacker(a), item(i), is_self_hit(self_hit) {
            is_about_self = about_self;
        }

        void draw() override {
            if (!can_draw()) { return; } 

            // Snap to the left, bottom, etc
            FeedConfig config = get_feed().get_config();
            INT32 hud_flags = config.hudflags;
            boolean reverse = config.position == FEED_POSITION_RIGHT;
            boolean center = config.position == FEED_POSITION_MIDDLE;

            // If the update doesn't concern the main player
            // then draw at a higher translucency
            INT32 low_priority_trans = (!is_about_self) ? 5 : -1;
            
            if (is_entering() || is_exiting()) {
                hud_flags |= (clamp_translucency(low_priority_trans) << V_ALPHASHIFT);
            } else {
                if (!is_about_self)
                    hud_flags |= (low_priority_trans << V_ALPHASHIFT);
            }
            
            // Config for drawing the patch
            c = FeedUpdateUtils::processItemConfig(item);

            if (center && !has_x_been_overwritten()) {
                // Take the total width of the update UP until the item graphic
                // subtract the x-coordinate by that width
                overwrite_x(x - (get_width_for_centering() * FRACUNIT));
            }

            // Prepare the drawer
            row = Draw(FixedToFloat(get_x()), FixedToFloat(y))
                .font(Draw::Font::kThin)
                .align(Draw::Align::kLeft)
                .flags(hud_flags);

            // Draw
            draw_background(config, static_cast<float>(get_background_width()));

            if (reverse) {
                draw_victim(reverse);
                draw_item(reverse); 
                draw_attacker(reverse);
            } else {
                draw_attacker(reverse);
                draw_item(reverse); 
                draw_victim(reverse);
            }
        }
};

// 2. Global event
/**
 * Shrink and SPBs are the only items that ends becoming everyone else's problem..
 * That warrants a bespoke update
 */

class GlobalFeedUpdate : public BaseFeedUpdate {
    private:
        std::string responsible {"Someone"}; // Player responsible
        std::string message {""};

        // Drawer
        Draw row;
        ProcessedItemConfig c;
        ItemConfigForFeedUpdate item {}; 

        UINT8* get_item_colormap() {
            return (c.rainbow) ? get_rainbow_colormap() : c.colormap;
        }

        void draw_message(boolean reverse) {
            int message_width = FeedUpdateUtils::getNameWidth(message);

            if (reverse) {
                row.x(-message_width).scale(FEED_UPDATE_SCALE).text(message.c_str());
            } else {
                row.scale(FEED_UPDATE_SCALE).text(message.c_str());
                row = row.x(message_width + c.x_offset).colormap(c.colormap);
            }
        }

        void draw_item(boolean reverse) {
            // Then the patch
            int x = 0;
            if (reverse) {
                x = -c.width_scaled;
            }
            
            row
                .x(x)
                .y(c.y_offset)
                .scale(c.patch_scale)
                .patch(c.patch_name);

            // Then the exclaimation point, because this is a big deal
            if (reverse) {
                row = row.x(-(c.width_scaled + c.x_offset)).colormap(static_cast<UINT8*>(NULL));
            } else {
                row = row.x(c.width_scaled + c.x_offset).colormap(static_cast<UINT8*>(NULL));
            }
        }

        void draw_exclaimation_point(boolean reverse) {
            if (reverse) {
                int exclaimation_width = FeedUpdateUtils::getNameWidth("!");
                row.x(-(exclaimation_width)).scale(FEED_UPDATE_SCALE).text("!");
                row = row.x(-(exclaimation_width + c.x_offset)).colormap(c.colormap);
            } else {
                row.scale(FEED_UPDATE_SCALE).text("!");
            }
        }

        int get_background_width () {
            // Message
            int message_width = FeedUpdateUtils::getNameWidth(message);
            int exclaimation_width = FeedUpdateUtils::getNameWidth("!");

            return (exclaimation_width + (c.width_scaled + c.x_offset)) + (message_width + c.x_offset);
        }
        
    public:
        // Constructor
        GlobalFeedUpdate(
            const std::string &s,
            const ItemConfigForFeedUpdate &i
        ): responsible(s), item(i) {
            // It's a global event, it DOES involve you
            is_about_self = true;
            message = fmt::format("{0} used", responsible);
        }

        void draw() override {
            if (!can_draw()) return;
            
            FeedConfig config = get_feed().get_config();
            INT32 hud_flags = config.hudflags;
            boolean reverse = config.position == FEED_POSITION_RIGHT;
            boolean center = config.position == FEED_POSITION_MIDDLE;
            
            if (is_entering() || is_exiting()) {
                hud_flags |= (updatetrans << V_ALPHASHIFT);
            }

            // Config for drawing the patch
            c = FeedUpdateUtils::processItemConfig(item);

            if (center && !has_x_been_overwritten()) {
                // Take the total width of the update
                // subtract the x-coordinate by half of that width
                overwrite_x(x - ((get_background_width()/2) * FRACUNIT));
            }

            // Prepare drawer
            row = Draw(FixedToFloat(get_x()), FixedToFloat(y))
                .font(Draw::Font::kThin)
                .align(Draw::Align::kLeft)
                .flags(hud_flags);
            
            // Background
            draw_background(config, static_cast<float>(get_background_width()));
            
            if (reverse) {
                draw_exclaimation_point(reverse);
                draw_item(reverse);
                draw_message(reverse);
            } else {
                draw_message(reverse);
                draw_item(reverse);
                draw_exclaimation_point(reverse);
            }
        }
};

// 3. Player event
/**
 * Events that pertain to only one player but aren't the result of a player interaction.
 * Be it falling in the pit, last place explosion, grades, etc.
 */
class GlobalPlayerFeedUpdate : public BaseFeedUpdate {
    private:
        std::string player {""};

        // Drawer
        Draw row;
        ProcessedItemConfig c;
        ItemConfigForFeedUpdate item {}; 

        UINT8* get_item_colormap() {
            if (c.rainbow)
                return get_rainbow_colormap();
            return (c.should_flash) ? c.colormap : c.item_recolormap;
        }

        void draw_player(boolean reverse) {
            int player_name_width = FeedUpdateUtils::getNameWidth(player);

            if (reverse) {
                row.x(-player_name_width).scale(FEED_UPDATE_SCALE).text(player.c_str());
                row = row.x(-(player_name_width + c.x_offset)).colormap(get_item_colormap());
            } else {
                row.scale(FEED_UPDATE_SCALE).text(player.c_str());
            }
        }

        void draw_patch(boolean reverse) {
            // Then the patch
            int x = 0;
            if (reverse) {
                x = -c.width_scaled;
            } else {
                row = row.colormap(get_item_colormap());
            }

            row
                .x(x)
                .y(c.y_offset)
                .scale(c.patch_scale)
                .patch(c.patch_name);

            // If in reverse, then the player name
            if (!reverse) {
                row = row.x(c.width_scaled + c.x_offset).colormap(static_cast<UINT8*>(NULL));
            }
        }

        int get_background_width() {
            // Message
            int player_width = FeedUpdateUtils::getNameWidth(player);
            return (c.width_scaled + c.x_offset) + (player_width);
        }
        
    public:
        // Constructor
        GlobalPlayerFeedUpdate(
            const std::string &s,
            const ItemConfigForFeedUpdate &i,
            const boolean &about_self
        ): player(s), item(i) {
            is_about_self = about_self;
        }

        void draw() override {
            if (!can_draw()) return;
            
            FeedConfig config = get_feed().get_config();
            INT32 hud_flags = config.hudflags;
            boolean reverse = config.position == FEED_POSITION_RIGHT;
            boolean center = config.position == FEED_POSITION_MIDDLE;

            // If the update doesn't concern the main player
            // then draw at a higher translucency
            INT32 low_priority_trans = (!is_about_self) ? 5 : -1;
            
            if (is_entering() || is_exiting()) {
                hud_flags |= (clamp_translucency(low_priority_trans) << V_ALPHASHIFT);
            } else {
                if (!is_about_self)
                    hud_flags |= (low_priority_trans << V_ALPHASHIFT);
            }

            // Config for drawing the patch
            c = FeedUpdateUtils::processItemConfig(item);

            if (center && !has_x_been_overwritten()) {
                // Take the total width of the update
                // subtract the x-coordinate by half of that width
                overwrite_x(x - ((get_background_width()/2) * FRACUNIT));
            }

            // Prepare drawer
            row = Draw(FixedToFloat(get_x()), FixedToFloat(y))
                .font(Draw::Font::kThin)
                .align(Draw::Align::kLeft)
                .flags(hud_flags);
            
            // Background
            draw_background(config, static_cast<float>(get_background_width()));
            
            if (reverse) {
                draw_player(reverse);
                draw_patch(reverse);
            } else {
                draw_patch(reverse);
                draw_player(reverse);
            }
        }
};

static FeedConfig getFeedConfig(void) {
    const FeedConfig default_config = (cv_rouletteonplayer.value == 1) ? NoItemBoxConfig : ItemBoxConfig;
    switch (cv_hudfeed_position.value) {
        case 0:
            return default_config;
        case 1:
            return TopMiddleConfig;
        case 2:
            return TimestampConfig;
        case 3:
            return BottomMiddleConfig;
        default:
            return default_config;
    }
    return default_config;
}

// Initialize the feed
static Hudfeed hudfeed(getFeedConfig());

static ItemConfigForFeedUpdate getItemConfigForFeedKillUpdate(mobj_t *mo) {
    mobjtype_t type = mo->type;
    switch (type) {
        case MT_SINK:
            return {"RRISSINK", 22, 16, {false, true}};
        default:
            break;
    }
    return {};
}

static ItemConfigForFeedUpdate getItemConfigForFeedDamageUpdate(mobj_t *mo) {
    mobjtype_t type = mo->type;
    
    switch (type) {
        case MT_ORBINAUT:
        case MT_ORBINAUT_SHIELD:
            return {"RRISORBN", 24, 22};
        case MT_JAWZ:
            return {"RRISJAWZ", 30, 20};
        case MT_BANANA:
        case MT_BANANA_SHIELD:
            return {"RRISBANA", 18, 19};
        case MT_SPBEXPLOSION: {
            ItemConfigForFeedUpdate spbConfig = {"RRISSPB", 21, 22, {false, true}, .y_offset = -2};
            if (mo->threshold) {
                if (mo->threshold == KITEM_EGGMAN) {
                    spbConfig = {"RRISEGGM", 26, 16, {false, true}, .y_offset = 0};
                }
            }
            return spbConfig;
        }
        case MT_GACHABOM:
            return {"RRISGBOM", 23, 22};
        case MT_LANDMINE:
            return {"RRISLNDM", 20, 20};
        case MT_BALLHOG:
        case MT_BALLHOGBOOM:
            return {"RRISBHOG", 26, 19};
        case MT_SSMINE:
        case MT_SSMINE_SHIELD:
            return {"RRISMINE", 22, 22, {false, true}};
        case MT_GARDENTOP:
            return {"RRISGTOP", 30, 19};
        case MT_INSTAWHIP:
            return {"RRINSTW", 55, 57, {true, false, 6, 2}, .15f};
        case MT_STONESHOE:
        case MT_STONESHOE_CHAIN:
            return {"RRISSHO", 23, 20};
        default:
            break;
    }

    return {};
}

const ItemConfigForFeedUpdate shrinkItemConfig = {
        .patch = "RRISSHRK", 
        .width = 12, 
        .height = 16, 
        .animation_config = {.should_flash = true, .flashing_colour = SKINCOLOR_ORANGE}, 
        .y_offset = 0
};
const ItemConfigForFeedUpdate dropTargetItemConfig = {
    .patch = "RRISDTRG", 
    .width = 30, 
    .height = 14
};
static ItemConfigForFeedUpdate getItemConfigForFeedAttackUpdate(playerattacks_t attack) {
    switch (attack) {
        case ATTACK_SNIPE:
            // Whenever the SEGA! jingle plays
            return {
                .patch = "RRFSNIP", 
                .width = 90, 
                .height = 31,
                .animation_config {
                    .animated = true,
                    .animated_frames = 2,
                },
                .patch_scale = 0.25f
            };
        case ATTACK_FLAMEDASH:
            return {"RRISFLMS", 18, 18};
        case ATTACK_HYUDORO:
            return {
                .patch = "RRISHYU",
                .width = 21,
                .height = 24,
                {
                    .animated = true,
                    .animated_frames = 9,
                    .tics_per_frame = 4
                },
                .y_offset = -2
            };
        case ATTACK_SHRINK:
            return shrinkItemConfig;
        case ATTACK_DROPTARGET:
            // When the drop target hasn't been hit at all
            return dropTargetItemConfig;
        case ATTACK_DROPTARGET_MEDIUM_HEALTH:
            // When the drop target has been hit once
            return {
                .patch = dropTargetItemConfig.patch,
                .width = dropTargetItemConfig.width,
                .height = dropTargetItemConfig.height,
                .recolour = SKINCOLOR_GOLD
            };
        case ATTACK_DROPTARGET_LOW_HEALTH:
            // When the drop target has been hit twice
            return {
                .patch = dropTargetItemConfig.patch,
                .width = dropTargetItemConfig.width,
                .height = dropTargetItemConfig.height,
                .recolour = SKINCOLOR_RED
            };
        case ATTACK_BUBBLESHIELD:
            return {"RRISBUBS", 18, 18};
        case ATTACK_BUBBLESHIELD_TRAP:
            return {"RRBLTRP", 31, 31, {true, false, 3, 8}, .25f};
        case ATTACK_LIGHTNING_SHIELD:
            // see PIT_LightningShieldAttack
            return {"RRISTHNS", 18, 18};
        case ATTACK_INVINCIBILITY:
            return {"RRISINV", 30, 27, {true, false, 6}, .y_offset = -2};
        case ATTACK_GROW:
            return {
                .patch = "RRISGROW", 
                .width = 16, 
                .height = 21, 
                {
                    .should_flash = true,
                    .flashing_colour = SKINCOLOR_BLUEBERRY
                }
            };
        case ATTACK_STONESHOE_TRAP:
            return {.patch = "RRISSHTR", .width = 30, .height = 26, .patch_scale = .4f, .y_offset = -2};
        case ATTACK_TOXOMISTER_CLOUD:
            return {"RRISTOXO", 23, 22};
        default:
            break;
    }
    return {};
}

static ItemConfigForFeedUpdate getItemConfigForGlobalFeedUpdate(globalfeedevent_t event) {
    switch (event) {
        case EVENT_SHRINK:
            return shrinkItemConfig;
        case EVENT_SPB:
            return {
                .patch = "RRSPBHR", 
                .width = 13, 
                .height = 17,
                .animation_config = {.animated = true, .animated_frames = 2, .tics_per_frame = 1},  
                .patch_scale = .5f,
            };
        default:
            break;
    }
    return {};
}

static playerattacks_t getPlayerAttackType(mobj_t* mo) {
    player_t* p = mo->player;

    if (p->flamedash > 0 && p->itemtype == KITEM_FLAMESHIELD) {
        return ATTACK_FLAMEDASH;
    }
    if (p->bubbleblowup > 0) {
        return ATTACK_BUBBLESHIELD;
    }
    if (p->invincibilitytimer > 0) {
        return ATTACK_INVINCIBILITY;
    }
    if(p->growshrinktimer > 0) {
        return ATTACK_GROW;
    }

    return ATTACK_NONE;
}

static boolean isPlayerMoValid(mobj_t *mo) {
    return (
        mo != NULL 
        && !P_MobjWasRemoved(mo)
        && mo->type == MT_PLAYER
        && mo->player != NULL
        && !mo->player->spectator
    );
}

static boolean arePlayersValid(mobj_t *m1, mobj_t *m2) {
    return isPlayerMoValid(m1) && isPlayerMoValid(m2);
}

static boolean isFeedUpdateAboutMainPlayer(player_t *p1, player_t* p2) {
    return stplyr->spectator == false && ((p1 == stplyr) || (p2 == stplyr));
}

static boolean canUseHudfeed(void) {
    return !RR_IsBattle() && radioracers_usehudfeed && cv_hudfeed_enabled.value;
}

// Push a player interaction to the feed.
void RR_PushPlayerDamageToFeed(mobj_t *source, mobj_t *target, mobj_t *inflictor) {
    if (!canUseHudfeed()) return;

    if (!inflictor || P_MobjWasRemoved(inflictor)) return;

    // If the inflictor type is a player, check for certain conditions
    if (inflictor->type == MT_PLAYER) {
        if (isPlayerMoValid(inflictor)) {
            RR_PushPlayerInteractionToFeed(source, target, getPlayerAttackType(inflictor));
        } else {
            return;
        }
    }

    // Special exception for MT_SPBEXPLOSION
    // Eggmark explosions use the same mobjtype, so check if the right player can be blamed
    if (inflictor->type == MT_SPBEXPLOSION) {
        if (inflictor->threshold == KITEM_EGGMAN) {
            if (isPlayerMoValid(inflictor->target)) {
                // True source (K_KartPlayerThink)
                source = inflictor->target;
            }
        }
    }

    // Check player mobjs
    if (!arePlayersValid(source, target)) return;

    // Verify if the item type is something the feed is configured to show
    ItemConfigForFeedUpdate itemConfig = getItemConfigForFeedDamageUpdate(inflictor);

    if (itemConfig.patch.empty()) return;

    player_t* source_plyr = source->player;
    player_t* target_plyr = target->player;

    // Amps are awarded to both the Stone Shoe owner AND the victim whenever someone gets damaged
    if(inflictor->type == MT_STONESHOE_CHAIN || inflictor->type == MT_STONESHOE) {
        player_t* stone_shoe_owner;
        if (inflictor->type == MT_STONESHOE_CHAIN) {
            stone_shoe_owner = Obj_StoneShoeChainShoeFollowPlayer(inflictor);
        } else {
            stone_shoe_owner = Obj_StoneShoeFollowPlayer(inflictor);
        }
        if (!stone_shoe_owner)
            return;
        
        source_plyr = stone_shoe_owner;
    }

    std::string attacker = player_names[source_plyr-players];
    std::string victim = player_names[target_plyr-players];

    // Cap off player names if they're too long
    // (maybe?)

    const boolean self_hit = source_plyr == target_plyr;

    // Push to the feed
    hudfeed.push(
        std::make_unique<PlayerFeedUpdate>(
            victim,
            attacker,
            itemConfig,
            self_hit,
            (self_hit || isFeedUpdateAboutMainPlayer(source_plyr, target_plyr))
        )
    );
}

void RR_PushPlayerDeathToFeed(mobj_t *source, mobj_t *target, mobj_t *inflictor) {
    if (!canUseHudfeed()) return;

    if (!inflictor || P_MobjWasRemoved(inflictor)) return;
    if (!arePlayersValid(source, target)) return;

    ItemConfigForFeedUpdate itemConfig = getItemConfigForFeedKillUpdate(inflictor);

    if (itemConfig.patch.empty()) return;

    player_t* source_plyr = source->player;
    player_t* target_plyr = target->player;

    std::string attacker = player_names[source_plyr-players];
    std::string victim = player_names[target_plyr-players];

    const boolean self_hit = source_plyr == target_plyr;
    
    // Push to the feed
    hudfeed.push(
        std::make_unique<PlayerFeedUpdate>(
            victim,
            attacker,
            itemConfig,
            self_hit,
            (self_hit || isFeedUpdateAboutMainPlayer(source_plyr, target_plyr))
        )
    );
}

void RR_PushPlayerInteractionToFeed(mobj_t *source, mobj_t *target, playerattacks_t attack) {
    if (!canUseHudfeed()) return;

    if (attack == ATTACK_NONE) return;
    if (attack == ATTACK_SNIPE && !cv_hudfeed_show_snipes.value) return; // So boring
    if (!arePlayersValid(source, target)) return;

    // Verify if the attack is something the feed is configured to show
    ItemConfigForFeedUpdate itemConfig = getItemConfigForFeedAttackUpdate(attack);

    if (itemConfig.patch.empty()) return;

    player_t* source_plyr = source->player;
    player_t* target_plyr = target->player;

    std::string attacker = player_names[source_plyr-players];
    std::string victim = player_names[target_plyr-players];

    // Cap off player names if they're too long
    // (maybe?)

    const boolean self_hit = source_plyr == target_plyr;

    // Push to the feed
    hudfeed.push(
        std::make_unique<PlayerFeedUpdate>(
            victim,
            attacker,
            itemConfig,
            self_hit,
            (self_hit || isFeedUpdateAboutMainPlayer(source_plyr, target_plyr))
        )
    );
}

void RR_PushGlobalEventToFeed(player_t* player, globalfeedevent_t event) {
    if (!canUseHudfeed()) return;

    ItemConfigForFeedUpdate itemConfig = getItemConfigForGlobalFeedUpdate(event);
    if (itemConfig.patch.empty()) return;
    
    const std::string player_name = player_names[player-players];

    // Push to the feed
    hudfeed.push(std::make_unique<GlobalFeedUpdate>(player_name, itemConfig));
}

void RR_PushGlobalGradeEventToFeed(player_t* player, gp_rank_e rank, boolean perfectRace) {
    if (!canUseHudfeed()) return;
    if (!cv_hudfeed_show_grades.value) return;
    if (rank == GRADE_INVALID) return;

    const boolean showSRanks = (cv_show_s_ranks.value && perfectRace);
    char grade_letter = (showSRanks) ? 'S' : K_GetGradeChar(rank);
    skincolornum_t grade_rank_colormap = static_cast<skincolornum_t>(K_GetGradeColor(rank));

    std::string grade_patch = fmt::format("R_CUPRN{0}", grade_letter);

    ItemConfigForFeedUpdate gradeConfig = {
        .patch = grade_patch,
        .width = 14,
        .height = 12,
        .patch_scale = .6f,
        .recolour = grade_rank_colormap,
        .rainbow = (showSRanks)
    };
        
    const std::string player_name = player_names[player-players];

    // Push to the feed
    hudfeed.push(std::make_unique<GlobalPlayerFeedUpdate>(
        player_name, 
        gradeConfig,
        stplyr == player
    ));
}

void RR_PushGlobalFaultEventToFeed(player_t* player) {
    if (!canUseHudfeed()) return;
    if (!cv_hudfeed_show_faults.value) return;

    // Big red 'X'
    ItemConfigForFeedUpdate faultConfig = {
        .patch = "K_NOBLNS",
        .width = 25,
        .height = 22,
    };

    // 20% chance
    if(M_RandomChance(FRACUNIT/20)) {
        faultConfig = {
            .patch = "RRHFFAUL",
            .width = 37,
            .height = 47,
            .patch_scale = .18f
        };
    }

    // Push to the feed
    hudfeed.push(std::make_unique<GlobalPlayerFeedUpdate>(
        player_names[player-players], 
        faultConfig,
        stplyr == player
    ));
}

// Draw the HUD feed
void RR_DrawHudFeed(void) {
    if (!has_feed_been_initialized) {
        RR_UpdateHudFeedConfig();
        has_feed_been_initialized = true;
        return;
    }
    if (!canUseHudfeed()) return;
    if (r_splitscreen > 0) return; // Never supporting splitscreen

    hudfeed.animate_feed();
}

// Update the HUD feed's timers
void RR_TickHudFeed(void) {
    if (!has_feed_been_initialized) return;
    if (!canUseHudfeed()) return;

    hudfeed.tick_feed();
}

// Wipe the feed whenever the player can't see it (i.e. during intermissions)
void RR_ClearHudFeed(void) {
    hudfeed.clear();
}

// Update the feed config on cvar change
void RR_UpdateHudFeedConfig(void) {
    hudfeed.clear();
    hudfeed.update_config(getFeedConfig());
}

static void dummyupdate(void) {
    ItemConfigForFeedUpdate bubble = {
        .patch = "RRFSNIP", 
        .width = 90, 
        .height = 31,
        .animation_config {
            .animated = true,
            .animated_frames = 2,
        },
        .patch_scale = 0.25f
    };
    // hudfeed.push(
    //     std::make_unique<PlayerFeedUpdate>(
    //         std::string("SUCMYADJKSAD"),
    //         std::string("$HOME"),
    //         bubble,
    //         false,
    //         true
    //     )
    // );
    // RR_PushGlobalEventToFeed(stplyr, EVENT_SPB);
    RR_PushGlobalGradeEventToFeed(stplyr, GRADE_S, true);
}

void RR_FeedCom(void) {
    COM_AddCommand("ff", dummyupdate);
}