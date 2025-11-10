// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_spawn.cpp (Game Entity Spawning)
// This file is responsible for parsing the entity data from a loaded map and
// spawning the corresponding entities into the game world. It acts as the
// bridge between the map editor's entity definitions and the in-game objects.
//
// Key Responsibilities:
// - Entity Parsing: The `ED_ParseEntity` function reads the key/value pairs
//   for each entity from the map's entity string.
// - Field Mapping: `ED_ParseField` maps the text-based keys from the map data
//   (e.g., "health", "speed") to the appropriate fields in the `gentity_t`
//   and `spawn_temp_t` structs.
// - Spawn Function Dispatch: `ED_CallSpawn` is the central function that looks
//   up an entity's `className` in a dispatch table and calls the correct `SP_*`
//   spawn function to initialize it.
// - Global Setup: The `SP_worldspawn` function is called for the first entity
//   in the map and is used to set up level-wide properties like the skybox,
//   music, and global game rules.

#include "../g_local.hpp"
#include "g_headhunters.hpp"
#include "../monsters/m_actor.hpp"
#include <sstream>	// for ent overrides
#include <fstream>	// for ent overrides
#include <algorithm>	// for std::fill

struct spawn_t {
	const char* name;
	void (*spawn)(gentity_t* ent);
};

const spawn_temp_t& ED_GetSpawnTemp()
{
	return st;
}

void SP_ambient_suck_wind(gentity_t* ent);
void SP_ambient_drone(gentity_t* ent);
void SP_ambient_flouro_buzz(gentity_t* ent);
void SP_ambient_drip(gentity_t* ent);
void SP_ambient_comp_hum(gentity_t* ent);
void SP_ambient_thunder(gentity_t* ent);
void SP_ambient_light_buzz(gentity_t* ent);
void SP_ambient_swamp1(gentity_t* ent);
void SP_ambient_swamp2(gentity_t* ent);
void SP_ambient_generic(gentity_t* ent);

void SP_info_player_start(gentity_t* ent);
void SP_info_player_deathmatch(gentity_t* ent);
void SP_info_player_team_red(gentity_t* self);
void SP_info_player_team_blue(gentity_t* self);
void SP_info_player_coop(gentity_t* ent);
void SP_info_player_coop_lava(gentity_t* self);
void SP_info_player_intermission(gentity_t* ent);
void SP_info_teleport_destination(gentity_t* self);
void SP_info_ctf_teleport_destination(gentity_t* self);
void SP_info_landmark(gentity_t* self);
void SP_info_world_text(gentity_t* self);
void SP_info_nav_lock(gentity_t* self);
void SP_team_redobelisk(gentity_t* ent);
void SP_team_blueobelisk(gentity_t* ent);
void SP_team_neutralobelisk(gentity_t* ent);

void SP_func_plat(gentity_t* ent);
void SP_func_plat2(gentity_t* ent);
void SP_func_rotating(gentity_t* ent);
void SP_func_rotating_ext(gentity_t* ent);	// oblivion support, although was still func_rotating in oblivion
void SP_func_button(gentity_t* ent);
void SP_func_door(gentity_t* ent);
void SP_func_door_secret(gentity_t* ent);
void SP_func_door_secret2(gentity_t* ent);
void SP_func_door_rotating(gentity_t* ent);
void SP_func_water(gentity_t* ent);
void SP_func_train(gentity_t* ent);
void SP_func_conveyor(gentity_t* self);
void SP_func_wall(gentity_t* self);
void SP_func_force_wall(gentity_t* ent);
void SP_func_object(gentity_t* self);
void SP_func_explosive(gentity_t* self);
void SP_func_timer(gentity_t* self);
void SP_func_areaportal(gentity_t* ent);
void SP_func_clock(gentity_t* ent);
void SP_func_killbox(gentity_t* ent);
void SP_func_eye(gentity_t* ent);
void SP_func_animation(gentity_t* ent);
void SP_func_spinning(gentity_t* ent);
void SP_object_repair(gentity_t* self);
void SP_func_bobbing(gentity_t* self);
void SP_func_pendulum(gentity_t* self);
void SP_func_illusionary(gentity_t* self);
void SP_func_rotate_train(gentity_t* ent);	// oblivion

void SP_trigger_always(gentity_t* ent);
void SP_trigger_once(gentity_t* ent);
void SP_trigger_multiple(gentity_t* ent);
void SP_trigger_relay(gentity_t* ent);
void SP_trigger_push(gentity_t* ent);
void SP_trigger_hurt(gentity_t* ent);
void SP_trigger_key(gentity_t* ent);
void SP_trigger_counter(gentity_t* ent);
void SP_trigger_elevator(gentity_t* ent);
void SP_trigger_gravity(gentity_t* ent);
void SP_trigger_monsterjump(gentity_t* ent);
void SP_trigger_flashlight(gentity_t* self);
void SP_trigger_fog(gentity_t* self);
void SP_trigger_coop_relay(gentity_t* self);
void SP_trigger_health_relay(gentity_t* self);
void SP_trigger_teleport(gentity_t* self);
void SP_trigger_ctf_teleport(gentity_t* self);
void SP_trigger_disguise(gentity_t* self);
void SP_trigger_safe_fall(gentity_t* self);

void SP_trigger_deathcount(gentity_t* ent);	//mm
void SP_trigger_no_monsters(gentity_t* ent);	//mm
void SP_trigger_monsters(gentity_t* ent);	//mm

void SP_trigger_misc_camera(gentity_t* ent);	// oblivion

void SP_target_temp_entity(gentity_t* ent);
void SP_target_speaker(gentity_t* ent);
void SP_target_explosion(gentity_t* ent);
void SP_target_changelevel(gentity_t* ent);
void SP_target_secret(gentity_t* ent);
void SP_target_goal(gentity_t* ent);
void SP_target_splash(gentity_t* ent);
void SP_target_spawner(gentity_t* ent);
void SP_target_blaster(gentity_t* ent);
void SP_target_crosslevel_trigger(gentity_t* ent);
void SP_target_crosslevel_target(gentity_t* ent);
void SP_target_crossunit_trigger(gentity_t* ent);
void SP_target_crossunit_target(gentity_t* ent);
void SP_target_laser(gentity_t* self);
void SP_target_help(gentity_t* ent);
void SP_target_actor(gentity_t* ent);
void SP_target_lightramp(gentity_t* self);
void SP_target_earthquake(gentity_t* ent);
void SP_target_character(gentity_t* ent);
void SP_target_string(gentity_t* ent);
void SP_target_camera(gentity_t* self);
void SP_target_gravity(gentity_t* self);
void SP_target_soundfx(gentity_t* self);
void SP_target_light(gentity_t* self);
void SP_target_poi(gentity_t* ent);
void SP_target_music(gentity_t* ent);
void SP_target_healthbar(gentity_t* self);
void SP_target_autosave(gentity_t* self);
void SP_target_sky(gentity_t* self);
void SP_target_achievement(gentity_t* self);
void SP_target_story(gentity_t* self);
void SP_target_mal_laser(gentity_t* ent);
void SP_target_steam(gentity_t* self);
void SP_target_anger(gentity_t* self);
void SP_target_killplayers(gentity_t* self);
// PMM - still experimental!
void SP_target_blacklight(gentity_t* self);
void SP_target_orb(gentity_t* self);
// pmm
void SP_target_remove_powerups(gentity_t* ent);	//q3
void SP_target_give(gentity_t* ent);	//q3
void SP_target_delay(gentity_t* ent);	//q3
void SP_target_print(gentity_t* ent);	//q3
void SP_target_teleporter(gentity_t* ent);	//q3
void SP_target_kill(gentity_t* self);	//q3
void SP_target_cvar(gentity_t* ent);	//ql
void SP_target_setskill(gentity_t* ent);
void SP_target_score(gentity_t* ent);	//q3
void SP_target_remove_weapons(gentity_t* ent);
void SP_target_railgun(gentity_t* ent);	// oblivion

void SP_target_shooter_grenade(gentity_t* ent);
void SP_target_shooter_rocket(gentity_t* ent);
void SP_target_shooter_bfg(gentity_t* ent);
void SP_target_shooter_prox(gentity_t* ent);
void SP_target_shooter_ionripper(gentity_t* ent);
void SP_target_shooter_phalanx(gentity_t* ent);
void SP_target_shooter_flechette(gentity_t* ent);

void SP_trap_shooter(gentity_t* ent);
void SP_trap_spikeshooter(gentity_t* ent);

void SP_target_push(gentity_t* ent);

void SP_worldspawn(gentity_t* ent);

void SP_dynamic_light(gentity_t* self);
void SP_rotating_light(gentity_t* self);
void SP_light(gentity_t* self);
void SP_light_mine1(gentity_t* ent);
void SP_light_mine2(gentity_t* ent);
void SP_info_null(gentity_t* self);
void SP_info_notnull(gentity_t* self);
void SP_misc_player_mannequin(gentity_t* self);
void SP_misc_model(gentity_t* self);
void SP_path_corner(gentity_t* self);
void SP_point_combat(gentity_t* self);

void SP_misc_explobox(gentity_t* self);
void SP_misc_banner(gentity_t* self);
void SP_misc_ctf_banner(gentity_t* ent);
void SP_misc_ctf_small_banner(gentity_t* ent);
void SP_misc_satellite_dish(gentity_t* self);
void SP_misc_actor(gentity_t* self);
void SP_domination_point(gentity_t* ent);
void SP_misc_gib_arm(gentity_t* self);
void SP_misc_gib_leg(gentity_t* self);
void SP_misc_gib_head(gentity_t* self);
void SP_misc_insane(gentity_t* self);
void SP_misc_deadsoldier(gentity_t* self);
void SP_misc_viper(gentity_t* self);
void SP_misc_viper_bomb(gentity_t* self);
void SP_misc_bigviper(gentity_t* self);
void SP_misc_strogg_ship(gentity_t* self);
void SP_misc_teleporter(gentity_t* self);
void SP_misc_teleporter_dest(gentity_t* self);
void SP_misc_blackhole(gentity_t* self);
void SP_misc_eastertank(gentity_t* self);
void SP_misc_easterchick(gentity_t* self);
void SP_misc_easterchick2(gentity_t* self);
void SP_misc_crashviper(gentity_t* ent);
void SP_misc_viper_missile(gentity_t* self);
void SP_misc_amb4(gentity_t* ent);
void SP_misc_transport(gentity_t* ent);
void SP_misc_nuke(gentity_t* ent);
void SP_misc_flare(gentity_t* ent);
void SP_misc_hologram(gentity_t* ent);
void SP_misc_lavaball(gentity_t* ent);
void SP_misc_nuke_core(gentity_t* self);
void SP_misc_camera(gentity_t* self);
void SP_misc_camera_target(gentity_t* self);

void SP_monster_berserk(gentity_t* self);
void SP_monster_gladiator(gentity_t* self);
void SP_monster_gunner(gentity_t* self);
void SP_monster_infantry(gentity_t* self);
void SP_monster_soldier_light(gentity_t* self);
void SP_monster_soldier(gentity_t* self);
void SP_monster_soldier_ss(gentity_t* self);
void SP_monster_tank(gentity_t* self);
void SP_monster_medic(gentity_t* self);
void SP_monster_flipper(gentity_t* self);
void SP_monster_eel(gentity_t* self);
void SP_monster_chick(gentity_t* self);
void SP_monster_parasite(gentity_t* self);
void SP_monster_flyer(gentity_t* self);
void SP_monster_brain(gentity_t* self);
void SP_monster_floater(gentity_t* self);
void SP_monster_hover(gentity_t* self);
void SP_monster_mutant(gentity_t* self);
void SP_monster_supertank(gentity_t* self);
void SP_monster_boss2(gentity_t* self);
void SP_monster_jorg(gentity_t* self);
void SP_monster_boss3_stand(gentity_t* self);
void SP_monster_makron(gentity_t* self);

void SP_monster_tank_stand(gentity_t* self);
void SP_monster_guardian(gentity_t* self);
void SP_monster_arachnid(gentity_t* self);
void SP_monster_guncmdr(gentity_t* self);

void SP_monster_commander_body(gentity_t* self);

void SP_turret_breach(gentity_t* self);
void SP_turret_base(gentity_t* self);
void SP_turret_driver(gentity_t* self);

void SP_monster_soldier_hypergun(gentity_t* self);
void SP_monster_soldier_lasergun(gentity_t* self);
void SP_monster_soldier_ripper(gentity_t* self);
void SP_monster_fixbot(gentity_t* self);
void SP_monster_gekk(gentity_t* self);
void SP_monster_chick_heat(gentity_t* self);
void SP_monster_gladb(gentity_t* self);
void SP_monster_boss5(gentity_t* self);

void SP_monster_stalker(gentity_t* self);
void SP_monster_turret(gentity_t* self);

void SP_hint_path(gentity_t* self);
void SP_monster_carrier(gentity_t* self);
void SP_monster_widow(gentity_t* self);
void SP_monster_widow2(gentity_t* self);
void SP_monster_kamikaze(gentity_t* self);
void SP_turret_invisible_brain(gentity_t* self);

//QUAKE
void SP_monster_shambler(gentity_t* self);

void SP_monster_dog(gentity_t* self);
void SP_monster_ogre(gentity_t* self);
void SP_monster_ogre_marksman(gentity_t* self);
void SP_monster_ogre_multigrenade(gentity_t* self);
void SP_monster_fish(gentity_t* self);
void SP_monster_army(gentity_t* self);
void SP_monster_centroid(gentity_t* self);
void SP_monster_fiend(gentity_t* self);
void SP_monster_zombie(gentity_t* self);
void SP_monster_spawn(gentity_t* self);
void SP_monster_spike(gentity_t* self);
void SP_monster_vore(gentity_t* self);
void SP_monster_overlord(gentity_t* self);
void SP_monster_wrath(gentity_t* self);
void SP_monster_enforcer(gentity_t* self);
void SP_monster_knight(gentity_t* self);
void SP_monster_sword(gentity_t* self);
void SP_monster_hell_knight(gentity_t* self);
void SP_monster_wizard(gentity_t* self);
void SP_monster_oldone(gentity_t* self);
void SP_monster_chthon(gentity_t* self);
void SP_monster_dragon(gentity_t* self);
void SP_monster_lavaman(gentity_t* self);
void SP_monster_boss(gentity_t* self);
void SP_monster_wyvern(gentity_t* self);

void SP_target_chthon_lightning(gentity_t* self);

// clang-format off
static const std::initializer_list<spawn_t> spawns = {
	{ "ambient_suck_wind", SP_ambient_suck_wind },
	{ "ambient_drone", SP_ambient_drone },
	{ "ambient_flouro_buzz", SP_ambient_flouro_buzz },
	{ "ambient_drip", SP_ambient_drip },
	{ "ambient_comp_hum", SP_ambient_comp_hum },
	{ "ambient_thunder", SP_ambient_thunder },
	{ "ambient_light_buzz", SP_ambient_light_buzz },
	{ "ambient_swamp1", SP_ambient_swamp1 },
	{ "ambient_swamp2", SP_ambient_swamp2 },
	{ "ambient_generic", SP_ambient_generic },

	{ "info_player_start", SP_info_player_start },
	{ "info_player_deathmatch", SP_info_player_deathmatch },
	{ "info_player_team_red", SP_info_player_team_red },
	{ "info_player_team_blue", SP_info_player_team_blue },
	{ "info_player_coop", SP_info_player_coop },
	{ "info_player_coop_lava", SP_info_player_coop_lava },
	{ "info_player_intermission", SP_info_player_intermission },
	{ "info_teleport_destination", SP_info_teleport_destination },
	{ "info_ctf_teleport_destination", SP_info_ctf_teleport_destination },
	{ "info_intermission", SP_info_player_intermission },
	{ "info_null", SP_info_null },
	{ "info_notnull", SP_info_notnull },
	{ "info_landmark", SP_info_landmark },
        { "info_world_text", SP_info_world_text },
        { "info_nav_lock", SP_info_nav_lock },
        { "domination_point", SP_domination_point },
        { "headhunters_receptacle", HeadHunters::SP_headhunters_receptacle },
        { "team_redobelisk", SP_team_redobelisk },
        { "team_blueobelisk", SP_team_blueobelisk },
        { "team_neutralobelisk", SP_team_neutralobelisk },

	{ "func_plat", SP_func_plat },
	{ "func_plat2", SP_func_plat2 },
	{ "func_button", SP_func_button },
	{ "func_door", SP_func_door },
	{ "func_door_secret", SP_func_door_secret },
	{ "func_door_secret2", SP_func_door_secret2 },
	{ "func_door_rotating", SP_func_door_rotating },
	{ "func_rotating", SP_func_rotating },
	{ "func_rotating_ext", SP_func_rotating_ext },
	{ "func_train", SP_func_train },
	{ "func_water", SP_func_water },
	{ "func_conveyor", SP_func_conveyor },
	{ "func_areaportal", SP_func_areaportal },
	{ "func_clock", SP_func_clock },
	{ "func_wall", SP_func_wall },
	{ "func_force_wall", SP_func_force_wall },
	{ "func_object", SP_func_object },
	{ "func_timer", SP_func_timer },
	{ "func_explosive", SP_func_explosive },
	{ "func_killbox", SP_func_killbox },
	{ "func_eye", SP_func_eye },
	{ "func_animation", SP_func_animation },
	{ "func_spinning", SP_func_spinning },
	{ "func_object_repair", SP_object_repair },
	{ "func_static", SP_func_wall },
	{ "func_bobbingwater", SP_func_water },
	{ "func_illusionary", SP_func_illusionary },
	{ "func_rotate_train", SP_func_rotate_train },

	{ "trigger_always", SP_trigger_always },
	{ "trigger_once", SP_trigger_once },
	{ "trigger_multiple", SP_trigger_multiple },
	{ "trigger_relay", SP_trigger_relay },
	{ "trigger_push", SP_trigger_push },
	{ "trigger_hurt", SP_trigger_hurt },
	{ "trigger_key", SP_trigger_key },
	{ "trigger_counter", SP_trigger_counter },
	{ "trigger_elevator", SP_trigger_elevator },
	{ "trigger_gravity", SP_trigger_gravity },
	{ "trigger_monsterjump", SP_trigger_monsterjump },
        { "trigger_flashlight", SP_trigger_flashlight },
        { "trigger_fog", SP_trigger_fog },
        { "trigger_coop_relay", SP_trigger_coop_relay },
        { "trigger_health_relay", SP_trigger_health_relay },
        { "trigger_teleport", SP_trigger_teleport },
        { "trigger_ctf_teleport", SP_trigger_ctf_teleport },
        { "trigger_disguise", SP_trigger_disguise },
        { "trigger_safe_fall", SP_trigger_safe_fall },
        { "trigger_setskill", SP_target_setskill },
        { "trigger_misc_camera", SP_trigger_misc_camera },
        { "trigger_proball_goal", SP_trigger_proball_goal },
        { "trigger_proball_oob", SP_trigger_proball_oob },

	{ "trigger_secret", SP_target_secret },

	{ "target_temp_entity", SP_target_temp_entity },
	{ "target_speaker", SP_target_speaker },
	{ "target_explosion", SP_target_explosion },
	{ "target_changelevel", SP_target_changelevel },
	{ "target_secret", SP_target_secret },
	{ "target_goal", SP_target_goal },
	{ "target_splash", SP_target_splash },
	{ "target_spawner", SP_target_spawner },
	{ "target_blaster", SP_target_blaster },
	{ "target_crosslevel_trigger", SP_target_crosslevel_trigger },
	{ "target_crosslevel_target", SP_target_crosslevel_target },
	{ "target_crossunit_trigger", SP_target_crossunit_trigger },
	{ "target_crossunit_target", SP_target_crossunit_target },
	{ "target_laser", SP_target_laser },
	{ "target_help", SP_target_help },
	{ "target_actor", SP_target_actor },
	{ "target_lightramp", SP_target_lightramp },
	{ "target_earthquake", SP_target_earthquake },
	{ "target_character", SP_target_character },
	{ "target_string", SP_target_string },
	{ "target_camera", SP_target_camera },
	{ "target_gravity", SP_target_gravity },
	{ "target_soundfx", SP_target_soundfx },
	{ "target_light", SP_target_light },
	{ "target_poi", SP_target_poi },
	{ "target_music", SP_target_music },
	{ "target_healthbar", SP_target_healthbar },
	{ "target_autosave", SP_target_autosave },
	{ "target_sky", SP_target_sky },
	{ "target_achievement", SP_target_achievement },
	{ "target_story", SP_target_story },
	{ "target_mal_laser", SP_target_mal_laser },
	{ "target_steam", SP_target_steam },
	{ "target_anger", SP_target_anger },
	{ "target_killplayers", SP_target_killplayers },
	// PMM - experiment
	{ "target_blacklight", SP_target_blacklight },
	{ "target_orb", SP_target_orb },
	// pmm
	{ "target_remove_powerups", SP_target_remove_powerups },
	{ "target_give", SP_target_give },
	{ "target_delay", SP_target_delay },
	{ "target_print", SP_target_print },
	{ "target_teleporter", SP_target_teleporter },
	{ "target_relay", SP_trigger_relay },
	{ "target_kill", SP_target_kill },
	{ "target_cvar", SP_target_cvar },
	{ "target_setskill", SP_target_setskill },
	{ "target_position", SP_info_notnull },
	{ "target_score", SP_target_score },
	{ "target_remove_weapons", SP_target_remove_weapons },

	{ "target_shooter_grenade", SP_target_shooter_grenade },
	{ "target_shooter_rocket", SP_target_shooter_rocket },
	{ "target_shooter_bfg", SP_target_shooter_bfg },
	{ "target_shooter_prox", SP_target_shooter_prox },
	{ "target_shooter_ionripper", SP_target_shooter_ionripper },
	{ "target_shooter_phalanx", SP_target_shooter_phalanx },
	{ "target_shooter_flechette", SP_target_shooter_flechette },
	{ "target_railgun", SP_target_railgun },	// oblivion compatibility

	{ "target_push", SP_target_push },

	{ "trap_shooter", SP_trap_shooter },
	{ "trap_spikeshooter", SP_trap_spikeshooter },

	{ "worldspawn", SP_worldspawn },

	{ "dynamic_light", SP_dynamic_light },
	{ "rotating_light", SP_rotating_light },
	{ "light", SP_light },
	{ "light_mine1", SP_light_mine1 },
	{ "light_mine2", SP_light_mine2 },
	{ "func_group", SP_info_null },
	{ "path_corner", SP_path_corner },
	{ "point_combat", SP_point_combat },

	{ "misc_explobox", SP_misc_explobox },
	{ "misc_banner", SP_misc_banner },
	{ "misc_ctf_banner", SP_misc_ctf_banner },
	{ "misc_ctf_small_banner", SP_misc_ctf_small_banner },
	{ "misc_satellite_dish", SP_misc_satellite_dish },
	{ "misc_actor", SP_misc_actor },
	{ "misc_player_mannequin", SP_misc_player_mannequin },
	{ "misc_model", SP_misc_model },
	{ "misc_gib_arm", SP_misc_gib_arm },
	{ "misc_gib_leg", SP_misc_gib_leg },
	{ "misc_gib_head", SP_misc_gib_head },
	{ "misc_insane", SP_misc_insane },
	{ "misc_deadsoldier", SP_misc_deadsoldier },
	{ "misc_viper", SP_misc_viper },
	{ "misc_viper_bomb", SP_misc_viper_bomb },
	{ "misc_bigviper", SP_misc_bigviper },
	{ "misc_strogg_ship", SP_misc_strogg_ship },
	{ "misc_teleporter", SP_misc_teleporter },
	{ "misc_teleporter_dest", SP_misc_teleporter_dest },
	{ "misc_blackhole", SP_misc_blackhole },
	{ "misc_eastertank", SP_misc_eastertank },
	{ "misc_easterchick", SP_misc_easterchick },
	{ "misc_easterchick2", SP_misc_easterchick2 },
	{ "misc_flare", SP_misc_flare },
	{ "misc_hologram", SP_misc_hologram },
	{ "misc_lavaball", SP_misc_lavaball },
	{ "misc_crashviper", SP_misc_crashviper },
	{ "misc_viper_missile", SP_misc_viper_missile },
	{ "misc_amb4", SP_misc_amb4 },
	{ "misc_transport", SP_misc_transport },
	{ "misc_nuke", SP_misc_nuke },
	{ "misc_nuke_core", SP_misc_nuke_core },
	{ "misc_camera", SP_misc_camera },		// oblivion
	{ "misc_camera_target", SP_misc_camera_target },	// oblivion

	{ "monster_berserk", SP_monster_berserk },
	{ "monster_gladiator", SP_monster_gladiator },
	{ "monster_gunner", SP_monster_gunner },
	{ "monster_infantry", SP_monster_infantry },
	{ "monster_soldier_light", SP_monster_soldier_light },
	{ "monster_soldier", SP_monster_soldier },
	{ "monster_soldier_ss", SP_monster_soldier_ss },
	{ "monster_tank", SP_monster_tank },
	{ "monster_tank_commander", SP_monster_tank },
	{ "monster_medic", SP_monster_medic },
	{ "monster_flipper", SP_monster_flipper },
	{ "monster_eel", SP_monster_eel },
	{ "monster_chick", SP_monster_chick },
	{ "monster_parasite", SP_monster_parasite },
	{ "monster_flyer", SP_monster_flyer },
	{ "monster_brain", SP_monster_brain },
	{ "monster_floater", SP_monster_floater },
	{ "monster_hover", SP_monster_hover },
	{ "monster_mutant", SP_monster_mutant },
	{ "monster_supertank", SP_monster_supertank },
	{ "monster_boss2", SP_monster_boss2 },
	{ "monster_boss3_stand", SP_monster_boss3_stand },
	{ "monster_jorg", SP_monster_jorg },
	{ "monster_makron", SP_monster_makron },
	{ "monster_tank_stand", SP_monster_tank_stand },
	{ "monster_guardian", SP_monster_guardian },
	{ "monster_arachnid", SP_monster_arachnid },
	{ "monster_guncmdr", SP_monster_guncmdr },

	{ "monster_commander_body", SP_monster_commander_body },

	{ "turret_breach", SP_turret_breach },
	{ "turret_base", SP_turret_base },
	{ "turret_driver", SP_turret_driver },

	{ "monster_soldier_hypergun", SP_monster_soldier_hypergun },
	{ "monster_soldier_lasergun", SP_monster_soldier_lasergun },
	{ "monster_soldier_ripper", SP_monster_soldier_ripper },
	{ "monster_fixbot", SP_monster_fixbot },
	{ "monster_gekk", SP_monster_gekk },
	{ "monster_chick_heat", SP_monster_chick_heat },
	{ "monster_gladb", SP_monster_gladb },
	{ "monster_boss5", SP_monster_boss5 },

	{ "monster_stalker", SP_monster_stalker },
	{ "monster_turret", SP_monster_turret },
	{ "monster_daedalus", SP_monster_hover },
	{ "hint_path", SP_hint_path },
	{ "monster_carrier", SP_monster_carrier },
	{ "monster_widow", SP_monster_widow },
	{ "monster_widow2", SP_monster_widow2 },
	{ "monster_medic_commander", SP_monster_medic },
	{ "monster_kamikaze", SP_monster_kamikaze },
	{ "turret_invisible_brain", SP_turret_invisible_brain },

	{ "monster_shambler", SP_monster_shambler },
	{ "monster_dog", SP_monster_dog },
	{ "monster_ogre", SP_monster_ogre },
	{ "monster_ogre_marksman", SP_monster_ogre_marksman },
	{ "monster_ogre_multigrenade", SP_monster_ogre_multigrenade },
	{ "monster_fish", SP_monster_fish },
	{ "monster_army", SP_monster_army },
	{ "monster_centroid", SP_monster_centroid },
	{ "monster_demon1", SP_monster_fiend },
	{ "monster_zombie", SP_monster_zombie },
	{ "monster_tarbaby", SP_monster_spawn },
	{ "monster_tarbaby_hell", SP_monster_spawn },
	{ "monster_spike", SP_monster_spike },
	{ "monster_spike_hell", SP_monster_spike },
	{ "monster_mine", SP_monster_spike },
	{ "monster_mine_hell", SP_monster_spike },
	{ "monster_shalrath", SP_monster_vore },
	{ "monster_enforcer", SP_monster_enforcer },
	{ "monster_knight", SP_monster_knight },
	{ "monster_sword", SP_monster_sword },
	{ "monster_hell_knight", SP_monster_hell_knight },
	{ "monster_wizard", SP_monster_wizard },
	{ "monster_oldone", SP_monster_oldone },
	{ "monster_chthon", SP_monster_chthon },
	{ "monster_dragon", SP_monster_dragon },
	{ "monster_lavaman", SP_monster_lavaman },
	{ "monster_boss", SP_monster_boss },
	{ "monster_wyvern", SP_monster_wyvern },

	{ "target_chthon_lightning", SP_target_chthon_lightning }
};
// clang-format on


static void SpawnEnt_MapFixes(gentity_t* ent) {
	if (!Q_strcasecmp(level.mapName.data(), "bunk1")) {
		if (!Q_strcasecmp(ent->className, "func_button") && !Q_strcasecmp(ent->model, "*36")) {
			ent->wait = -1;
		}
		return;
	}
	if (!Q_strcasecmp(level.mapName.data(), "q64/dm7")) {
		if (ent->s.origin == Vector3{ 1056, 1056, 40 } && !Q_strcasecmp(ent->className, "info_player_deathmatch")) {
			// silly location, move this spawn point back away from the lava trap
			ent->s.origin = Vector3{ 1312, 928, 40 };
		}
		return;
	}
	if (!Q_strcasecmp(ent->className, "item_health_mega")) {
		if (!Q_strcasecmp(level.mapName.data(), "q2dm1")) {
			if (ent->s.origin == Vector3{ 480, 1376, 912 }) {
				ent->s.angles = { 0, -45, 0 };
			}
			return;
		}
		if (!Q_strcasecmp(level.mapName.data(), "q2dm8")) {
			if (ent->s.origin == Vector3{ -832, 192, -232 }) {
				ent->s.angles = { 0, 90, 0 };
			}
			return;
		}
		if (!Q_strcasecmp(level.mapName.data(), "fact3")) {
			if (ent->s.origin == Vector3{ -80, 568, 144 }) {
				ent->s.angles = { 0, -90, 0 };
			}
			return;
		}
	}
}

// ----------

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn(gentity_t* ent) {

	if (!ent->className) {
		gi.Com_PrintFmt("{}: nullptr className\n", __FUNCTION__);
		FreeEntity(ent);
		return;
	}

	// do this before calling the spawn function so it can be overridden.
	ent->gravityVector[0] = 0.0;
	ent->gravityVector[1] = 0.0;
	ent->gravityVector[2] = -1.0;

	ent->sv.init = false;

#if 0
	if (Game::Is(GameType::Horde)) {
		// remove monsters from map, we will spawn them in during wave starts
		if (!strnicmp(ent->className, "monster_", 8)) {
			FreeEntity(ent);
			return;
		}
	}
#endif
	// FIXME - PMM classnames hack
	if (!strcmp(ent->className, "weapon_nailgun"))
		ent->className = GetItemByIndex(IT_WEAPON_ETF_RIFLE)->className;
	else if (!strcmp(ent->className, "ammo_nails"))
		ent->className = GetItemByIndex(IT_AMMO_FLECHETTES)->className;
	else if (!strcmp(ent->className, "weapon_heatbeam"))
		ent->className = GetItemByIndex(IT_WEAPON_PLASMABEAM)->className;
	else if (!strcmp(ent->className, "item_haste"))
		ent->className = GetItemByIndex(IT_POWERUP_HASTE)->className;
	else if (RS(Quake3Arena) && !strcmp(ent->className, "weapon_supershotgun"))
		ent->className = GetItemByIndex(IT_WEAPON_SHOTGUN)->className;
	else if (!strcmp(ent->className, "info_player_team1"))
		ent->className = "info_player_team_red";
	else if (!strcmp(ent->className, "info_player_team2"))
		ent->className = "info_player_team_blue";
	else if (!strcmp(ent->className, "item_flag_team1"))
		ent->className = ITEM_CTF_FLAG_RED;
	else if (!strcmp(ent->className, "item_flag_team2"))
		ent->className = ITEM_CTF_FLAG_BLUE;

	if (RS(Quake1)) {
		if (!strcmp(ent->className, "weapon_machinegun"))
			ent->className = GetItemByIndex(IT_WEAPON_ETF_RIFLE)->className;
		else if (!strcmp(ent->className, "weapon_chaingun"))
			ent->className = GetItemByIndex(IT_WEAPON_PLASMABEAM)->className;
		else if (!strcmp(ent->className, "weapon_railgun"))
			ent->className = GetItemByIndex(IT_WEAPON_HYPERBLASTER)->className;
		else if (!strcmp(ent->className, "ammo_slugs"))
			ent->className = GetItemByIndex(IT_AMMO_CELLS)->className;
		else if (!strcmp(ent->className, "ammo_bullets"))
			ent->className = GetItemByIndex(IT_AMMO_FLECHETTES)->className;
		else if (!strcmp(ent->className, "ammo_grenades"))
			ent->className = GetItemByIndex(IT_AMMO_ROCKETS_SMALL)->className;
	}
	// pmm

	SpawnEnt_MapFixes(ent);

	// check item spawn functions
	for (size_t index = static_cast<size_t>(IT_NULL + 1); index < itemList.size(); ++index) {
		Item* item = &itemList[index];
		if (!item->className)
			continue;
		if (!strcmp(item->className, ent->className)) {
			// found it
			// before spawning, pick random item replacement
			if (g_dm_random_items->integer) {
				ent->item = item;
				item_id_t new_item = DoRandomRespawn(ent);

				if (new_item) {
					item = GetItemByIndex(new_item);
					ent->className = item->className;
				}
			}

			SpawnItem(ent, item);
			return;
		}
	}

	// check normal spawn functions
	for (auto& s : spawns) {
		if (!strcmp(s.name, ent->className)) { // found it
			s.spawn(ent);

			if (strcmp(ent->className, s.name) == 0)
				ent->className = s.name;

			if (deathmatch->integer && !ent->saved) {
				saved_spawn_t* spawn = (saved_spawn_t*)gi.TagMalloc(sizeof(saved_spawn_t), TAG_LEVEL);
				*spawn = {
					ent->s.origin,
					ent->s.angles,
					ent->health,
					ent->dmg,
					ent->s.scale,
					ent->target,
					ent->targetName,
					ent->spawnFlags,
					ent->mass,
					ent->className,
					ent->mins,
					ent->maxs,
					ent->model,
					s.spawn
				};
				ent->saved = spawn;
			}
			return;
		}
	}

	if (!strcmp(ent->className, "item_ball")) {
		if (Game::Is(GameType::ProBall)) {
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderFX |= RF_SHELL_RED | RF_SHELL_GREEN;
		}
		else {
			FreeEntity(ent);
		}
		return;
	}

	gi.Com_PrintFmt("{}: {} doesn't have a spawn function.\n", __FUNCTION__, *ent);
	FreeEntity(ent);
}

/*
=============
ED_NewString
=============
*/
char* ED_NewString(const char* string) {
	char* newb, * new_p;
	int		i;
	size_t	l;

	l = strlen(string) + 1;

	newb = (char*)gi.TagMalloc(l, TAG_LEVEL);

	new_p = newb;

	for (i = 0; i < l; i++) {
		if (string[i] == '\\' && i < l - 1) {
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

//
// fields are used for spawning from the entity string
//

struct field_t {
	const char* name;
	void (*load_func) (gentity_t* e, const char* s) = nullptr;
};

// utility template for getting the type of a field
template<typename>
struct member_object_container_type {};
template<typename T1, typename T2>
struct member_object_container_type<T1 T2::*> { using type = T2; };
template<typename T>
using member_object_container_type_t = typename member_object_container_type<std::remove_cv_t<T>>::type;

struct type_loaders_t {
	template<typename T, std::enable_if_t<std::is_same_v<T, const char*>, int> = 0>
	static T load(const char* s) {
		return ED_NewString(s);
	}

	template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	static T load(const char* s) {
		return atoi(s);
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, SpawnFlags>, int> = 0>
	static T load(const char* s) {
		return SpawnFlags(atoi(s));
	}

	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	static T load(const char* s) {
		return atof(s);
	}

	template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
	static T load(const char* s) {
		if constexpr (sizeof(T) > 4)
			return static_cast<T>(atoll(s));
		else
			return static_cast<T>(atoi(s));
	}

	template<typename T, std::enable_if_t<std::is_same_v<T, Vector3>, int> = 0>
	static T load(const char* s) {
		Vector3 vec{};
		static char vec_buffer[32];
		const char* token = COM_Parse(&s, vec_buffer, sizeof(vec_buffer));
		vec.x = atof(token);
		token = COM_Parse(&s);
		vec.y = atof(token);
		token = COM_Parse(&s);
		vec.z = atof(token);
		return vec;
	}

	template<typename T>
	struct is_char_std_array : std::false_type {};

	template<size_t N>
	struct is_char_std_array<std::array<char, N>> : std::true_type {};

	/*
	===============
	type_loaders_t::load  (std::array<char, N>)
	===============
	*/
	template<typename T, std::enable_if_t<is_char_std_array<T>::value, int> = 0>
	static T load(const char* s) {
		T out{};                       // value-initialize, fills with '\0'
		if (s && *s) {
			Q_strlcpy(out.data(), s, out.size());
		}
		return out;
	}
};

#define AUTO_LOADER_FUNC(M) \
	[](gentity_t *e, const char *s) { \
		e->M = type_loaders_t::load<decltype(e->M)>(s); \
	}

static int32_t ED_LoadColor(const char* value) {
	// space means rgba as values
	if (strchr(value, ' ')) {
		static char color_buffer[32];
		std::array<float, 4> raw_values{ 0, 0, 0, 1.0f };
		bool is_float = true;

		for (auto& v : raw_values) {
			const char* token = COM_Parse(&value, color_buffer, sizeof(color_buffer));

			if (*token) {
				v = atof(token);

				if (v > 1.0f)
					is_float = false;
			}
		}

		if (is_float)
			for (auto& v : raw_values)
				v *= 255.f;

		return ((int32_t)raw_values[3]) | (((int32_t)raw_values[2]) << 8) | (((int32_t)raw_values[1]) << 16) | (((int32_t)raw_values[0]) << 24);
	}

	// integral
	return atoi(value);
}

#define FIELD_COLOR(n, x) \
	{ n, [](gentity_t *e, const char *s) { \
		e->x = ED_LoadColor(s); \
	} }

// clang-format off
// fields that get copied directly to gentity_t
#define FIELD_AUTO(x) \
	{ #x, AUTO_LOADER_FUNC(x) }

#define FIELD_AUTO_NAMED(n, x) \
	{ n, AUTO_LOADER_FUNC(x) }

static const std::initializer_list<field_t> entity_fields = {
	FIELD_AUTO(className),
	FIELD_AUTO(model),
	FIELD_AUTO(spawnFlags),
	FIELD_AUTO(speed),
	FIELD_AUTO(accel),
	FIELD_AUTO(decel),
	FIELD_AUTO(target),
	FIELD_AUTO(targetName),
	FIELD_AUTO(pathTarget),
	FIELD_AUTO(deathTarget),
	FIELD_AUTO(healthTarget),
	FIELD_AUTO(itemTarget),
	FIELD_AUTO(killTarget),
	FIELD_AUTO(combatTarget),
	FIELD_AUTO(message),
	FIELD_AUTO(team),
	FIELD_AUTO(wait),
	FIELD_AUTO(delay),
	FIELD_AUTO(random),
	FIELD_AUTO_NAMED("move_origin", moveOrigin),
	FIELD_AUTO_NAMED("move_angles", moveAngles),
	FIELD_AUTO(style),
	FIELD_AUTO(style_on),
	FIELD_AUTO(style_off),
	FIELD_AUTO(crosslevel_flags),
	FIELD_AUTO(count),
	FIELD_AUTO(health),
	FIELD_AUTO(sounds),
	{ "light" },
	FIELD_AUTO(dmg),
	FIELD_AUTO(mass),
	FIELD_AUTO(volume),
	FIELD_AUTO(attenuation),
	FIELD_AUTO(map),
	FIELD_AUTO_NAMED("origin", s.origin),
	FIELD_AUTO_NAMED("angles", s.angles),
	{ "angle", [](gentity_t* e, const char* value) {
		e->s.angles = {};
		e->s.angles[YAW] = atof(value);
	} },
	FIELD_COLOR("rgba", s.skinNum),
	FIELD_AUTO(hackFlags),
	FIELD_AUTO_NAMED("alpha", s.alpha),
	FIELD_AUTO_NAMED("scale", s.scale),
	FIELD_AUTO_NAMED("mangle", mangle),
	FIELD_AUTO_NAMED("dead_frame", monsterInfo.startFrame),
	FIELD_AUTO_NAMED("frame", s.frame),
	FIELD_AUTO_NAMED("effects", s.effects),
	FIELD_AUTO_NAMED("renderFX", s.renderFX),

	// [Paril-KEX] fog keys
	FIELD_AUTO_NAMED("fog_color", fog.color),
	FIELD_AUTO_NAMED("fog_color_off", fog.color_off),
	FIELD_AUTO_NAMED("fog_density", fog.density),
	FIELD_AUTO_NAMED("fog_density_off", fog.density_off),
	FIELD_AUTO_NAMED("fog_sky_factor", fog.sky_factor),
	FIELD_AUTO_NAMED("fog_sky_factor_off", fog.sky_factor_off),

	FIELD_AUTO_NAMED("heightfog_falloff", heightfog.falloff),
	FIELD_AUTO_NAMED("heightfog_density", heightfog.density),
	FIELD_AUTO_NAMED("heightfog_start_color", heightfog.start_color),
	FIELD_AUTO_NAMED("heightfog_start_dist", heightfog.start_dist),
	FIELD_AUTO_NAMED("heightfog_end_color", heightfog.end_color),
	FIELD_AUTO_NAMED("heightfog_end_dist", heightfog.end_dist),

	FIELD_AUTO_NAMED("heightfog_falloff_off", heightfog.falloff_off),
	FIELD_AUTO_NAMED("heightfog_density_off", heightfog.density_off),
	FIELD_AUTO_NAMED("heightfog_start_color_off", heightfog.start_color_off),
	FIELD_AUTO_NAMED("heightfog_start_dist_off", heightfog.start_dist_off),
	FIELD_AUTO_NAMED("heightfog_end_color_off", heightfog.end_color_off),
	FIELD_AUTO_NAMED("heightfog_end_dist_off", heightfog.end_dist_off),

	// [Paril-KEX] func_eye stuff
	FIELD_AUTO_NAMED("eye_position", moveOrigin),
	FIELD_AUTO_NAMED("vision_cone", yawSpeed),

	// [Paril-KEX] for trigger_coop_relay
	FIELD_AUTO_NAMED("message2", map),
	FIELD_AUTO(mins),
	FIELD_AUTO(maxs),

	// [Paril-KEX] customizable bmodel animations
	FIELD_AUTO_NAMED("bmodel_anim_start", bmodel_anim.start),
	FIELD_AUTO_NAMED("bmodel_anim_end", bmodel_anim.end),
	FIELD_AUTO_NAMED("bmodel_anim_style", bmodel_anim.style),
	FIELD_AUTO_NAMED("bmodel_anim_speed", bmodel_anim.speed),
	FIELD_AUTO_NAMED("bmodel_anim_nowrap", bmodel_anim.nowrap),

	FIELD_AUTO_NAMED("bmodel_anim_alt_start", bmodel_anim.alt_start),
	FIELD_AUTO_NAMED("bmodel_anim_alt_end", bmodel_anim.alt_end),
	FIELD_AUTO_NAMED("bmodel_anim_alt_style", bmodel_anim.alt_style),
	FIELD_AUTO_NAMED("bmodel_anim_alt_speed", bmodel_anim.alt_speed),
	FIELD_AUTO_NAMED("bmodel_anim_alt_nowrap", bmodel_anim.alt_nowrap),

	// [Paril-KEX] customizable power armor stuff
	FIELD_AUTO_NAMED("powerArmorPower", monsterInfo.powerArmorPower),
	{ "powerArmorType", [](gentity_t* s, const char* v) {
			int32_t type = atoi(v);

			if (type == 0)
				s->monsterInfo.powerArmorType = IT_NULL;
			else if (type == 1)
				s->monsterInfo.powerArmorType = IT_POWER_SCREEN;
			else
				s->monsterInfo.powerArmorType = IT_POWER_SHIELD;
		}
	},

	//muff
		FIELD_AUTO(gametype),
		FIELD_AUTO(not_gametype),
		FIELD_AUTO(notteam),
		FIELD_AUTO(notfree),
		FIELD_AUTO(notq2),
		FIELD_AUTO(notq3a),
		FIELD_AUTO(notarena),
		FIELD_AUTO(ruleset),
		FIELD_AUTO(not_ruleset),
		FIELD_AUTO(powerups_on),
		FIELD_AUTO(powerups_off),
		FIELD_AUTO(bfg_on),
		FIELD_AUTO(bfg_off),
		FIELD_AUTO(plasmabeam_on),
		FIELD_AUTO(plasmabeam_off),
		FIELD_AUTO(spawnpad),
		FIELD_AUTO(height),
		FIELD_AUTO(phase),
		FIELD_AUTO(bob),
		FIELD_AUTO(duration),
		FIELD_AUTO(bobFrame),
	//-muff
		FIELD_AUTO_NAMED("rotate", moveAngles),
		FIELD_AUTO_NAMED("speeds", moveOrigin),
		FIELD_AUTO_NAMED("durations", durations),

		FIELD_AUTO_NAMED("monster_slots", monsterInfo.monster_slots)
};

#undef AUTO_LOADER_FUNC

#define AUTO_LOADER_FUNC(M) \
	[](spawn_temp_t *e, const char *s) { \
		e->M = type_loaders_t::load<decltype(e->M)>(s); \
	}

struct temp_field_t {
	const char* name;
	void (*load_func) (spawn_temp_t* e, const char* s) = nullptr;
};

// temp spawn vars -- only valid when the spawn function is called
// (copied to `st`)
static const std::initializer_list<temp_field_t> temp_fields = {
	FIELD_AUTO(lip),
	FIELD_AUTO(distance),
	FIELD_AUTO(height),
	FIELD_AUTO(noise),
	FIELD_AUTO(pauseTime),
	FIELD_AUTO(item),

	FIELD_AUTO(gravity),
	FIELD_AUTO(sky),
	FIELD_AUTO(skyRotate),
	FIELD_AUTO(skyAxis),
	FIELD_AUTO(skyAutoRotate),
	FIELD_AUTO(minYaw),
	FIELD_AUTO(maxYaw),
	FIELD_AUTO(minPitch),
	FIELD_AUTO(maxPitch),
	FIELD_AUTO(nextMap),
	FIELD_AUTO(music),  // [Edward-KEX]
	FIELD_AUTO(instantItems),
	FIELD_AUTO(radius),
	FIELD_AUTO(hub_map),
	FIELD_AUTO(achievement),

	FIELD_AUTO_NAMED("shadowlightradius", sl.data.radius),
	FIELD_AUTO_NAMED("shadowlightresolution", sl.data.resolution),
	FIELD_AUTO_NAMED("shadowlightintensity", sl.data.intensity),
	FIELD_AUTO_NAMED("shadowlightstartfadedistance", sl.data.fadeStart),
	FIELD_AUTO_NAMED("shadowlightendfadedistance", sl.data.fadeEnd),
	FIELD_AUTO_NAMED("shadowlightstyle", sl.data.lightStyle),
	FIELD_AUTO_NAMED("shadowlightconeangle", sl.data.coneAngle),
	FIELD_AUTO_NAMED("shadowlightstyletarget", sl.lightStyleTarget),

	FIELD_AUTO(goals),

	FIELD_AUTO(image),

	FIELD_AUTO(fade_start_dist),
	FIELD_AUTO(fade_end_dist),
	FIELD_AUTO(start_items),
	FIELD_AUTO(no_grapple),
	FIELD_AUTO(no_dm_spawnpads),
	FIELD_AUTO(no_dm_telepads),
	FIELD_AUTO(health_multiplier),

	FIELD_AUTO(reinforcements),
	FIELD_AUTO(noise_start),
	FIELD_AUTO(noise_middle),
	FIELD_AUTO(noise_end),

	FIELD_AUTO(loop_count),

	FIELD_AUTO(cvar),
	FIELD_AUTO(cvarValue),
	FIELD_AUTO(author),
	FIELD_AUTO(author2),

	FIELD_AUTO(ruleset),

	FIELD_AUTO(noBots),
	FIELD_AUTO(noHumans),
	FIELD_AUTO(arena)
};
// clang-format on

/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an entity
===============
*/
static void ED_ParseField(const char* key, const char* value, gentity_t* ent) {

	// check st first
	for (auto& f : temp_fields) {
		if (Q_strcasecmp(f.name, key))
			continue;

		st.keys_specified.emplace(f.name);

		// found it
		if (f.load_func)
			f.load_func(&st, value);

		return;
	}

	// now entity
	for (auto& f : entity_fields) {
		if (Q_strcasecmp(f.name, key))
			continue;

		st.keys_specified.emplace(f.name);

		// [Paril-KEX]
		if (!strcmp(f.name, "bmodel_anim_start") || !strcmp(f.name, "bmodel_anim_end"))
			ent->bmodel_anim.enabled = true;

		// found it
		if (f.load_func)
			f.load_func(ent, value);

		return;
	}

	//gi.Com_PrintFmt("{} is not a valid field\n", key);
}

/*
====================
ED_ParseEntity

Parses an entity out of the given string, returning the new position
ed should be a properly initialized empty entity.
====================
*/
static const char* ED_ParseEntity(const char* data, gentity_t* ent) {
	bool  init;
	char  keyname[256];
	const char* com_token;

	init = false;
	st = {};

	// go through all the dictionary pairs
	while (1) {
		// parse key
		com_token = COM_Parse(&data);
		if (com_token[0] == '}')
			break;
		if (!data)
			gi.Com_Error("ED_ParseEntity: EOF without closing brace");

		Q_strlcpy(keyname, com_token, sizeof(keyname));

		// parse value
		com_token = COM_Parse(&data);
		if (!data)
			gi.Com_Error("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			gi.Com_Error("ED_ParseEntity: closing brace without data");

		init = true;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_') {
			// [Sam-KEX] Hack for setting RGBA for shadow-casting lights
			if (!strcmp(keyname, "_color"))
				ent->s.skinNum = ED_LoadColor(com_token);

			continue;
		}

		ED_ParseField(keyname, com_token, ent);
	}

	if (!init)
		memset(ent, 0, sizeof(*ent));

	return data;
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamChain field set to the next one
================
*/

// adjusts teams so that trains that move their children
// are in the front of the team
static void G_FixTeams() {
	gentity_t* e, * e2, * chain;
	uint32_t i, j;
	uint32_t c;

	c = 0;
	for (i = 1, e = g_entities + i; i < globals.numEntities; i++, e++) {
		if (!e->inUse)
			continue;
		if (!e->team)
			continue;
		if (!strcmp(e->className, "func_train") && e->spawnFlags.has(SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN)) {
			if (e->flags & FL_TEAMSLAVE) {
				chain = e;
				e->teamMaster = e;
				e->teamChain = nullptr;
				e->flags &= ~FL_TEAMSLAVE;
				e->flags |= FL_TEAMMASTER;
				c++;
				for (j = 1, e2 = g_entities + j; j < globals.numEntities; j++, e2++) {
					if (e2 == e)
						continue;
					if (!e2->inUse)
						continue;
					if (!e2->team)
						continue;
					if (!strcmp(e->team, e2->team)) {
						chain->teamChain = e2;
						e2->teamMaster = e;
						e2->teamChain = nullptr;
						chain = e2;
						e2->flags |= FL_TEAMSLAVE;
						e2->flags &= ~FL_TEAMMASTER;
						e2->moveType = MoveType::Push;
						e2->speed = e->speed;
					}
				}
			}
		}
	}

	if (c)
		gi.Com_PrintFmt("{}: {} entity team{} repaired.\n", __FUNCTION__, c, c != 1 ? "s" : "");
}

static void G_FindTeams() {
	gentity_t* e1, * e2, * chain;
	uint32_t i, j;
	uint32_t c1, c2;

	c1 = 0;
	c2 = 0;
	for (i = 1, e1 = g_entities + i; i < globals.numEntities; i++, e1++) {
		if (!e1->inUse)
			continue;
		if (!e1->team)
			continue;
		if (e1->flags & FL_TEAMSLAVE)
			continue;
		chain = e1;
		e1->teamMaster = e1;
		e1->flags |= FL_TEAMMASTER;
		c1++;
		c2++;
		for (j = i + 1, e2 = e1 + 1; j < globals.numEntities; j++, e2++) {
			if (!e2->inUse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e1->team, e2->team)) {
				c2++;
				chain->teamChain = e2;
				e2->teamMaster = e1;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	G_FixTeams();

	if (c1 && g_verbose->integer)
		gi.Com_PrintFmt("{}: {} entity team{} found with a total of {} entit{}.\n", __FUNCTION__, c1, c1 != 1 ? "s" : "", c2, c2 != 1 ? "ies" : "y");
}

// inhibit entities from game based on cvars & spawnFlags
static inline bool G_InhibitEntity(gentity_t* ent) {
	if (ent->gametype) {
		const char* s = strstr(ent->gametype, Game::GetCurrentInfo().spawn_name.data());
		if (!s)
			return true;
	}
	if (ent->not_gametype) {
		const char* s = strstr(ent->not_gametype, Game::GetCurrentInfo().spawn_name.data());
		if (s)
			return true;
	}

	if (ent->notteam && Teams())
		return true;
	if (ent->notfree && !Teams())
		return true;

	if (ent->notq2 && RS(Quake2))
		return true;
	if (ent->notq3a && RS(Quake3Arena))
		return true;
	if (ent->notarena && (Game::Has(GameFlags::Arena)))
		return true;

	if (ent->powerups_on && !game.map.spawnPowerups)
		return true;
	if (ent->powerups_off && !!game.map.spawnPowerups)
		return true;

	if (ent->bfg_on && !game.map.spawnBFG)
		return true;
	if (ent->bfg_off && !!game.map.spawnBFG)
		return true;

	if (ent->plasmabeam_on && !game.map.spawnPlasmaBeam)
		return true;
	if (ent->plasmabeam_off && !!game.map.spawnPlasmaBeam)
		return true;

	if (ent->spawnpad && ent->spawnpad[0]) {
		if (!strcmp(ent->spawnpad, "pu")) {
			if (!game.map.spawnPowerups)
				return true;
		}
		if (!strcmp(ent->spawnpad, "ar")) {
			if (!game.map.spawnArmor)
				return true;
		}
		if (!strcmp(ent->spawnpad, "ht")) {
			if (!game.map.spawnHealth || g_vampiric_damage->integer)
				return true;
		}
	}

	if (ent->ruleset && *ent->ruleset) {
		std::string_view includeStr{ ent->ruleset };
		for (const auto& alias : rs_short_name[static_cast<size_t>(game.ruleset)]) {
			if (!alias.empty() && includeStr.find(alias) != std::string_view::npos)
				goto skip_not_ruleset;
		}
		return true; // no matching alias in ruleset -> inhibit entity
	}

skip_not_ruleset:
	if (ent->not_ruleset && *ent->not_ruleset) {
		std::string_view excludeStr{ ent->not_ruleset };
		for (const auto& alias : rs_short_name[static_cast<size_t>(game.ruleset)]) {
			if (!alias.empty() && excludeStr.find(alias) != std::string_view::npos)
				return true; // matched in not_ruleset -> inhibit entity
		}
	}

	// dm-only
	if (deathmatch->integer)
		return ent->spawnFlags.has(SPAWNFLAG_NOT_DEATHMATCH);

	// coop flags
	if (coop->integer && ent->spawnFlags.has(SPAWNFLAG_NOT_COOP))
		return true;
	else if (!coop->integer && ent->spawnFlags.has(SPAWNFLAG_COOP_ONLY))
		return true;

	if (g_quadhog->integer && !strcmp(ent->className, "item_quad"))
		return true;

	// skill
	return ((skill->integer == 0) && ent->spawnFlags.has(SPAWNFLAG_NOT_EASY)) ||
		((skill->integer == 1) && ent->spawnFlags.has(SPAWNFLAG_NOT_MEDIUM)) ||
		((skill->integer >= 2) && ent->spawnFlags.has(SPAWNFLAG_NOT_HARD));
}

void setup_shadow_lights();

// [Paril-KEX]
void PrecacheInventoryItems() {
	if (deathmatch->integer)
		return;

	for (auto ce : active_clients()) {
		for (item_id_t id = IT_NULL; id != IT_TOTAL; id = static_cast<item_id_t>(id + 1))
			if (ce->client->pers.inventory[id])
				PrecacheItem(GetItemByIndex(id));
	}
}

static void PrecacheStartItems() {
	const char* raw = g_start_items && g_start_items->string ? g_start_items->string : "";
	if (*raw == '\0') {
		return;
	}

	using std::string;
	using std::string_view;

	auto ltrim = [](string_view sv) -> string_view {
		size_t i = 0;
		while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t' || sv[i] == '\r' || sv[i] == '\n')) { ++i; }
		return sv.substr(i);
		};
	auto rtrim = [](string_view sv) -> string_view {
		size_t i = sv.size();
		while (i > 0) {
			char c = sv[i - 1];
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { --i; }
			else { break; }
		}
		return sv.substr(0, i);
		};
	auto trim = [&](string_view sv) -> string_view { return rtrim(ltrim(sv)); };

	auto next_token_ws = [](string_view sv) -> std::pair<string_view, string_view> {
		// split on ASCII whitespace
		size_t i = 0;
		while (i < sv.size() && sv[i] != ' ' && sv[i] != '\t' && sv[i] != '\r' && sv[i] != '\n') { ++i; }
		string_view head = sv.substr(0, i);
		// skip whitespace to remainder
		while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t' || sv[i] == '\r' || sv[i] == '\n')) { ++i; }
		return { head, sv.substr(i) };
		};

	string_view all{ raw };
	// Walk semicolon-separated entries
	while (!all.empty()) {
		// Find next ';'
		size_t pos = all.find(';');
		string_view entry = pos == string_view::npos ? all : all.substr(0, pos);
		all = pos == string_view::npos ? string_view{} : all.substr(pos + 1);

		entry = trim(entry);
		if (entry.empty()) {
			continue;
		}

		// First token is the item classname; ignore any extra fields
		auto split = next_token_ws(entry);
		string_view item_sv = trim(split.first);
		if (item_sv.empty()) {
			continue;
		}

		// Ensure null-terminated for legacy APIs
		string item_name{ item_sv };

		Item* item = FindItemByClassname(item_name.c_str());
		if (!item || !item->pickup) {
			gi.Com_ErrorFmt("Invalid g_start_item entry: {}\n", item_name.c_str());
			// Com_ErrorFmt should not return, but guard just in case
			continue;
		}

		PrecacheItem(item);
	}
}

static void PrecachePlayerSounds() {

	gi.soundIndex("player/lava1.wav");
	gi.soundIndex("player/lava2.wav");

	gi.soundIndex("player/gasp1.wav"); // gasping for air
	gi.soundIndex("player/gasp2.wav"); // head breaking surface, not gasping

	gi.soundIndex("player/watr_in.wav");  // feet hitting water
	gi.soundIndex("player/watr_out.wav"); // feet leaving water

	gi.soundIndex("player/watr_un.wav"); // head going underwater

	gi.soundIndex("player/u_breath1.wav");
	gi.soundIndex("player/u_breath2.wav");

	gi.soundIndex("player/wade1.wav");
	gi.soundIndex("player/wade2.wav");
	gi.soundIndex("player/wade3.wav");
	gi.soundIndex("misc/talk1.wav");

	gi.soundIndex("world/land.wav");   // landing thud
	gi.soundIndex("misc/h2ohit1.wav"); // landing splash

	// gibs
	gi.soundIndex("misc/udeath.wav");

	gi.soundIndex("items/respawn1.wav");
	gi.soundIndex("misc/mon_power2.wav");

	// sexed sounds
	gi.soundIndex("*death1.wav");
	gi.soundIndex("*death2.wav");
	gi.soundIndex("*death3.wav");
	gi.soundIndex("*death4.wav");
	gi.soundIndex("*fall1.wav");
	gi.soundIndex("*fall2.wav");
	gi.soundIndex("*gurp1.wav"); // drowning damage
	gi.soundIndex("*gurp2.wav");
	gi.soundIndex("*jump1.wav"); // player jump
	gi.soundIndex("*pain25_1.wav");
	gi.soundIndex("*pain25_2.wav");
	gi.soundIndex("*pain50_1.wav");
	gi.soundIndex("*pain50_2.wav");
	gi.soundIndex("*pain75_1.wav");
	gi.soundIndex("*pain75_2.wav");
	gi.soundIndex("*pain100_1.wav");
	gi.soundIndex("*pain100_2.wav");
	gi.soundIndex("*drown1.wav");
}

void GT_PrecacheAssets() {
	if (Teams()) {
		if (Game::IsNot(GameType::RedRover)) {
			ii_teams_header_red = gi.imageIndex("tag4");
			ii_teams_header_blue = gi.imageIndex("tag5");
		}
		ii_teams_red_default = gi.imageIndex("i_ctf1");
		ii_teams_blue_default = gi.imageIndex("i_ctf2");
		ii_teams_red_tiny = gi.imageIndex("sbfctf1");
		ii_teams_blue_tiny = gi.imageIndex("sbfctf2");
	}

	if (Game::Has(GameFlags::OneVOne))
		ii_duel_header = gi.imageIndex("/tags/default");

	if (Game::Has(GameFlags::CTF)) {
		ii_ctf_red_dropped = gi.imageIndex("i_ctf1d");
		ii_ctf_blue_dropped = gi.imageIndex("i_ctf2d");
		ii_ctf_red_taken = gi.imageIndex("i_ctf1t");
		ii_ctf_blue_taken = gi.imageIndex("i_ctf2t");
		mi_ctf_red_flag = gi.modelIndex("players/male/flag1.md2");
		mi_ctf_blue_flag = gi.modelIndex("players/male/flag2.md2");
	}
}

// [Paril-KEX]
static void PrecacheAssets() {
	if (!deathmatch->integer) {
		gi.soundIndex("infantry/inflies1.wav");

		// help icon for statusbar
		gi.imageIndex("i_help");
		gi.imageIndex("help");
		gi.soundIndex("misc/pc_up.wav");
	}

	level.picPing = gi.imageIndex("loc_ping");

	level.picHealth = gi.imageIndex("i_health");
	gi.imageIndex("field_3");

	gi.soundIndex("items/pkup.wav");   // bonus item pickup

	//gi.soundIndex("items/damage.wav");
	//gi.soundIndex("items/protect.wav");
	//gi.soundIndex("items/protect4.wav");
	gi.soundIndex("weapons/noammo.wav");
	gi.soundIndex("weapons/lowammo.wav");
	gi.soundIndex("weapons/change.wav");

	// gibs
	sm_meat_index.assign("models/objects/gibs/sm_meat/tris.md2");
	gi.modelIndex("models/objects/gibs/arm/tris.md2");
	gi.modelIndex("models/objects/gibs/bone/tris.md2");
	gi.modelIndex("models/objects/gibs/bone2/tris.md2");
	gi.modelIndex("models/objects/gibs/chest/tris.md2");
	gi.modelIndex("models/objects/gibs/skull/tris.md2");
	gi.modelIndex("models/objects/gibs/head2/tris.md2");
	gi.modelIndex("models/objects/gibs/sm_metal/tris.md2");

	ii_highlight = gi.imageIndex("i_ctfj");

	GT_PrecacheAssets();

	gi.soundIndex("misc/talk1.wav");
}

constexpr int MAX_READ = 0x10000;		// read in blocks of 64k
static void FS_Read(void* buffer, int len, FILE* f) {
	int		block, remaining;
	int		read;
	byte* buf;
	int		tries;

	buf = (byte*)buffer;

	remaining = len;
	tries = 0;
	while (remaining) {
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = fread(buf, 1, block, f);
		if (read == 0) {
			if (!tries) {
				tries = 1;
			}
			else
				gi.Com_Error("FS_Read: 0 bytes read");
		}

		if (read == -1)
			gi.Com_Error("FS_Read: -1 bytes read");

		remaining -= read;
		buf += read;
	}
}


/*
==============
VerifyEntityString
==============
*/
static bool VerifyEntityString(const char* entities) {
	const char* or_token;
	gentity_t* or_ent = nullptr;
	const char* or_buf = entities;
	bool		or_error = false;

	while (1) {
		// parse the opening brace
		or_token = COM_Parse(&or_buf);
		if (!or_buf)
			break;
		if (or_token[0] != '{') {
			gi.Com_PrintFmt("{}: Found \"{}\" when expecting {{ in override.\n", __FUNCTION__, or_token);
			return false;
		}

		while (1) {
			// parse key
			or_token = COM_Parse(&or_buf);
			if (or_token[0] == '}')
				break;
			if (!or_buf) {
				gi.Com_ErrorFmt("{}: EOF without closing brace.\n", __FUNCTION__);
				return false;
			}
			// parse value
			or_token = COM_Parse(&or_buf);
			if (!or_buf) {
				gi.Com_ErrorFmt("{}: EOF without closing brace.\n", __FUNCTION__);
				return false;
			}
			if (or_token[0] == '}') {
				gi.Com_ErrorFmt("{}: Closing brace without data.\n", __FUNCTION__);
				return false;
			}
		}

	}
	return true;
}

static void PrecacheForRandomRespawn() {
	for (auto& item : itemList) {
		if (!item.flags || (item.flags & (IF_NOT_GIVEABLE | IF_TECH | IF_NOT_RANDOM)) || !item.pickup || !item.worldModel)
			continue;

		PrecacheItem(&item);
	}
}

/*
==============
MapPostProcess
==============
*/
static void MapPostProcess(gentity_t* ent) {
	if (!strcmp(level.mapName.data(), "bunk1") && !strcmp(ent->className, "func_button") && !Q_strcasecmp(ent->model, "*36")) {
		ent->wait = -1;
	}
}

/*
==============
TryLoadEntityOverride
==============
*/
static const char* TryLoadEntityOverride(const char* mapName, const char* default_entities) {
	std::string overridePath = std::string(G_Fmt("baseq2/{}/{}.ent",
		g_entityOverrideDir->string[0] ? g_entityOverrideDir->string : "maps",
		mapName).data());

	// Try to load override
	if (g_entityOverrideLoad->integer && !strstr(mapName, ".dm2")) {
		std::ifstream in(overridePath, std::ios::binary | std::ios::ate);
		if (in) {
			std::streamsize size = in.tellg();
			if (size > 0 && size <= 0x40000) {
				in.seekg(0, std::ios::beg);
				std::vector<char> buffer(size + 1);
				if (in.read(buffer.data(), size)) {
					buffer[size] = '\0';
					if (VerifyEntityString(buffer.data())) {
						if (g_verbose->integer)
							gi.Com_PrintFmt("{}: Entities override file verified and loaded: \"{}\"\n", __FUNCTION__, overridePath.c_str());
						char* out = (char*)gi.TagMalloc(buffer.size(), TAG_GAME);
						memcpy(out, buffer.data(), buffer.size());
						return out;
					}
				}
			}
			else {
				gi.Com_PrintFmt("{}: Entities override file too large: \"{}\"\n", __FUNCTION__, overridePath.c_str());
			}
		}
	}

	// Save override if not present
	if (g_entityOverrideSave->integer && !strstr(mapName, ".dm2")) {
		std::ifstream test(overridePath);
		if (!test) {
			std::ofstream out(overridePath, std::ios::binary);
			if (out) {
				out.write(default_entities, strlen(default_entities));
				if (g_verbose->integer)
					gi.Com_PrintFmt("{}: Entities override file written to: \"{}\"\n", __FUNCTION__, overridePath.c_str());
			}
		}
	}

	return default_entities;
}

/*
===============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
===============
*/
void SpawnEntities(const char* mapName, const char* entities, const char* spawnPoint) {
	if (entities && *entities) {
		entities = TryLoadEntityOverride(mapName, entities);
		level.savedEntityString.assign(entities);
	}
	else {
		if (g_verbose->integer) {
			gi.Com_PrintFmt("{}: Empty entity string for map \"{}\".\n", __FUNCTION__, mapName);
		}

		level.savedEntityString.clear();
		static const char emptyEntityString[] = "";
		entities = emptyEntityString;
	}

	// Clamp skill level to valid range [0, 4]
	const int skillLevel = std::clamp(skill->integer, 0, 4);
	if (skill->integer != skillLevel)
		gi.cvarForceSet("skill", G_Fmt("{}", skillLevel).data());

	// Clear cached asset indices
	cached_soundIndex::clear_all();
	cached_modelIndex::clear_all();
	cached_imageIndex::clear_all();

	// Reset all persistent game state
        SaveClientData();
        gi.FreeTags(TAG_LEVEL);
        level = LevelLocals{};
        Domination_ClearState();
        HeadHunters::ClearState();
        ProBall::ClearState();
        ProBall::ClearState();
        level.entityReloadGraceUntil = level.time + FRAME_TIME_MS * 2;
	std::memset(g_entities, 0, sizeof(g_entities[0]) * game.maxEntities);

	globals.serverFlags &= SERVER_FLAG_LOADING;

	Q_strlcpy(level.mapName.data(), mapName, level.mapName.size());
	if (!game.autoSaved) {
		const char* src = spawnPoint ? spawnPoint : "";
		const size_t cap = game.spawnPoint.size();
		const size_t n = std::min(std::strlen(src), cap - 1);
		std::memcpy(game.spawnPoint.data(), src, n);
		game.spawnPoint[n] = '\0';
	}

	std::string_view mapView(level.mapName.data(), strnlen(level.mapName.data(), level.mapName.size()));
	level.isN64 = mapView.starts_with("q64/");
	level.campaign.coopScalePlayers = 0;
	level.campaign.coopHealthScaling = std::clamp(g_coop_health_scaling->value, 0.0f, 1.0f);

	// Initialize all client structs
	for (size_t i = 0; i < game.maxClients; ++i) {
		g_entities[i + 1].client = &game.clients[i];
		game.clients[i].pers.connected = false;
		game.clients[i].pers.limitedLivesPersist = false;
		game.clients[i].pers.limitedLivesStash = 0;
		game.clients[i].pers.spawned = false;
	}

	InitBodyQue();

	int inhibited = 0;
	bool firstEntity = true;

	while (true) {
		const char* token = COM_Parse(&entities);
		if (!entities || token[0] == '\0')
			break;

		if (token[0] != '{') {
			gi.Com_ErrorFmt("{}: Found \"{}\" when expecting {{ in entity string.\n", __FUNCTION__, token);
		}

		gentity_t* ent = firstEntity ? g_entities : Spawn();
		firstEntity = false;

		entities = ED_ParseEntity(entities, ent);

		if (ent != g_entities) {
			if (G_InhibitEntity(ent)) {
				FreeEntity(ent);
				++inhibited;
				continue;
			}
			ent->spawnFlags &= ~SPAWNFLAG_EDITOR_MASK;
		}

		ent->gravityVector = { 0.0f, 0.0f, -1.0f };
		ED_CallSpawn(ent);
		MapPostProcess(ent);
		ent->s.renderFX |= RF_IR_VISIBLE;
	}

	if (inhibited > 0 && g_verbose->integer) {
		gi.Com_PrintFmt("{} entities inhibited.\n", inhibited);
	}

	// Level post-processing and setup
	PrecacheStartItems();
	PrecacheInventoryItems();
	G_FindTeams();
	QuadHog_SetupSpawn(5_sec);
	Tech_SetupSpawn();

	if (deathmatch->integer) {
		if (g_dm_random_items->integer) {
			PrecacheForRandomRespawn();
		}
		game.item_inhibit_pu = 0;
		game.item_inhibit_pa = 0;
		game.item_inhibit_ht = 0;
		game.item_inhibit_ar = 0;
		game.item_inhibit_am = 0;
		game.item_inhibit_wp = 0;
	}
	else {
		InitHintPaths();
	}

        G_LocateSpawnSpots();
        setup_shadow_lights();

        Domination_InitLevel();
        HeadHunters::InitLevel();
        ProBall::InitLevel();
        ProBall::InitLevel();

        level.init = true;
}


bool G_ResetWorldEntitiesFromSavedString() {
	if (level.savedEntityString.empty())
		return false;

	level.entityReloadGraceUntil = level.time + FRAME_TIME_MS * 2;

	for (size_t i = static_cast<size_t>(game.maxClients) + BODY_QUEUE_SIZE + 1; i < globals.numEntities; ++i) {
		gentity_t* ent = &g_entities[i];
		if (!ent->inUse)
			continue;

		FreeEntity(ent);
	}

	gi.FreeTags(TAG_LEVEL);

	level.spawn.Clear();
        level.spawnSpots.fill(nullptr);
        level.shadowLightCount = 0;
        std::fill(level.shadowLightInfo.begin(), level.shadowLightInfo.end(), ShadowLightInfo{});
        level.campaign = {};
        level.start_items = nullptr;
        level.instantItems = false;
        level.no_grapple = false;
        level.no_dm_spawnpads = false;
        level.no_dm_telepads = false;
        level.timeoutOwner = nullptr;

        Domination_ClearState();
        HeadHunters::ClearState();

	globals.numEntities = game.maxClients + 1;

	std::memset(world, 0, sizeof(*world));
	world->s.number = 0;

	level.bodyQue = 0;
	InitBodyQue();

	const char* entities = level.savedEntityString.c_str();
	bool firstEntity = true;
	int inhibited = 0;

	while (true) {
		const char* token = COM_Parse(&entities);
		if (!entities || token[0] == '\0')
			break;

		if (token[0] != '{') {
			gi.Com_ErrorFmt("{}: Found \"{}\" when expecting opening brace in entity string.\n", __FUNCTION__, token);
		}

		gentity_t* ent = firstEntity ? g_entities : Spawn();
		firstEntity = false;

		entities = ED_ParseEntity(entities, ent);

		if (ent != g_entities) {
			if (G_InhibitEntity(ent)) {
				FreeEntity(ent);
				++inhibited;
				continue;
			}
			ent->spawnFlags &= ~SPAWNFLAG_EDITOR_MASK;
		}

		ent->gravityVector = { 0.0f, 0.0f, -1.0f };
		ED_CallSpawn(ent);
		MapPostProcess(ent);
		ent->s.renderFX |= RF_IR_VISIBLE;
	}

	if (inhibited > 0 && g_verbose->integer) {
		gi.Com_PrintFmt("{} entities inhibited.\n", inhibited);
	}

	PrecacheStartItems();
	PrecacheInventoryItems();
	G_FindTeams();
	QuadHog_SetupSpawn(5_sec);
	Tech_SetupSpawn();

	if (deathmatch->integer) {
		if (g_dm_random_items->integer) {
			PrecacheForRandomRespawn();
		}
		game.item_inhibit_pu = 0;
		game.item_inhibit_pa = 0;
		game.item_inhibit_ht = 0;
		game.item_inhibit_ar = 0;
		game.item_inhibit_am = 0;
		game.item_inhibit_wp = 0;
	}
	else {
		InitHintPaths();
	}

        G_LocateSpawnSpots();
        setup_shadow_lights();

        Domination_InitLevel();
        HeadHunters::InitLevel();

        level.init = true;

	return true;
}

//===================================================================

#include "g_statusbar.hpp"

// create & set the statusbar string for the current gamemode

/*
=========================
AddCombatHUD
=========================
*/
static void AddCombatHUD(statusbar_t& sb) {
	// Health, ammo, armor, selected icon
	sb.ifstat(STAT_SHOW_STATUSBAR)
		.xv(0).hnum().xv(50).pic(STAT_HEALTH_ICON)
		.ifstat(STAT_AMMO_ICON).xv(100).anum().xv(150).pic(STAT_AMMO_ICON).endifstat()
		.ifstat(STAT_ARMOR_ICON).xv(200).rnum().xv(250).pic(STAT_ARMOR_ICON).endifstat()
		.ifstat(STAT_SELECTED_ICON).xv(296).pic(STAT_SELECTED_ICON).endifstat()
		.endifstat();

	sb.yb(-50);

	// Pickup + selected item name
	sb.ifstat(STAT_SHOW_STATUSBAR)
		.ifstat(STAT_PICKUP_ICON).xv(0).pic(STAT_PICKUP_ICON).xv(26).yb(-42).loc_stat_string(STAT_PICKUP_STRING).yb(-50).endifstat()
		.ifstat(STAT_SELECTED_ITEM_NAME).yb(-34).xv(319).loc_stat_rstring(STAT_SELECTED_ITEM_NAME).yb(-58).endifstat()
		.endifstat();

	// Help icon
	sb.ifstat(STAT_SHOW_STATUSBAR).ifstat(STAT_HELPICON).xv(150).pic(STAT_HELPICON).endifstat().endifstat();
}

/*
=========================
AddPowerupsAndTech
=========================
*/
static void AddPowerupsAndTech(statusbar_t& sb) {
	sb.ifstat(STAT_SHOW_STATUSBAR)
		.ifstat(STAT_POWERUP_ICON).xv(262).num(2, STAT_POWERUP_TIME).xv(296).pic(STAT_POWERUP_ICON).endifstat()
		.ifstat(STAT_TECH).yb(-137).xr(-26).pic(STAT_TECH).endifstat()
		.endifstat();
}

/*
=========================
AddCoopStatus
=========================
*/
static void AddCoopStatus(statusbar_t& sb) {
	sb.ifstat(STAT_COOP_RESPAWN).xv(0).yt(0).loc_stat_cstring2(STAT_COOP_RESPAWN).endifstat();

	int y = 2;
	const int step = 26;
	if (G_LimitedLivesActive())
		sb.ifstat(STAT_LIVES).xr(-16).yt(y).lives_num(STAT_LIVES).xr(0).yt(y += step).loc_rstring("$g_lives").endifstat();

	if (Game::Is(GameType::Horde)) {
		int n = level.roundNumber;
		int chars = n > 99 ? 3 : n > 9 ? 2 : 1;
		sb.ifstat(STAT_ROUND_NUMBER).xr(-32 - (16 * chars)).yt(y += 10).num(3, STAT_ROUND_NUMBER).xr(0).yt(y += step).loc_rstring("Wave").endifstat();

		n = level.campaign.totalMonsters - level.campaign.killedMonsters;
		chars = n > 99 ? 3 : n > 9 ? 2 : 1;
		sb.ifstat(STAT_MONSTER_COUNT).xr(-32 - (16 * chars)).yt(y += 10).num(3, STAT_MONSTER_COUNT).xr(0).yt(y += step).loc_rstring("Monsters").endifstat();
	}
}

/*
=========================
AddSPExtras
=========================
*/
static void AddSPExtras(statusbar_t& sb) {
	// Key icons, powerup icon reflow, selected item name offset
	sb.ifstat(STAT_POWERUP_ICON).yb(-76).endifstat();
	sb.ifstat(STAT_SELECTED_ITEM_NAME)
		.yb(-58)
		.ifstat(STAT_POWERUP_ICON).yb(-84).endifstat()
		.endifstat();

	sb.ifstat(STAT_KEY_A).xv(296).pic(STAT_KEY_A).endifstat();
	sb.ifstat(STAT_KEY_B).xv(272).pic(STAT_KEY_B).endifstat();
	sb.ifstat(STAT_KEY_C).xv(248).pic(STAT_KEY_C).endifstat();

	sb.ifstat(STAT_HEALTH_BARS).yt(24).health_bars().endifstat();

	sb.story();
}

/*
=========================
AddDeathmatchStatus
=========================
*/
static void AddDeathmatchStatus(statusbar_t& sb) {
	if (Teams()) {
		if (Game::Has(GameFlags::CTF))
			sb.ifstat(STAT_CTF_FLAG_PIC).xr(-24).yt(26).pic(STAT_CTF_FLAG_PIC).endifstat();

		sb.ifstat(STAT_TEAMPLAY_INFO).xl(0).yb(-88).stat_string(STAT_TEAMPLAY_INFO).endifstat();
	}

	sb.ifstat(STAT_COUNTDOWN).xv(136).yb(-256).num(3, STAT_COUNTDOWN).endifstat();
	sb.ifstat(STAT_MATCH_STATE).xv(0).yb(-78).stat_string(STAT_MATCH_STATE).endifstat();

	sb.ifstat(STAT_FOLLOWING).xv(0).yb(-68).string2("FOLLOWING").xv(80).stat_string(STAT_FOLLOWING).endifstat();
	sb.ifstat(STAT_SPECTATOR).xv(0).yb(-68).string2("SPECTATING").xv(0).yb(-58).string("Use TAB Menu to join the match.").xv(80).endifstat();

	sb.ifstat(STAT_MINISCORE_FIRST_PIC).xr(-26).yb(-110).pic(STAT_MINISCORE_FIRST_PIC).xr(-78).num(3, STAT_MINISCORE_FIRST_SCORE).ifstat(STAT_MINISCORE_FIRST_VAL).xr(-24).yb(-94).stat_string(STAT_MINISCORE_FIRST_VAL).endifstat().endifstat();
	sb.ifstat(STAT_MINISCORE_FIRST_POS).xr(-28).yb(-112).pic(STAT_MINISCORE_FIRST_POS).endifstat();
	sb.ifstat(STAT_MINISCORE_SECOND_PIC).xr(-26).yb(-83).pic(STAT_MINISCORE_SECOND_PIC).xr(-78).num(3, STAT_MINISCORE_SECOND_SCORE).ifstat(STAT_MINISCORE_SECOND_VAL).xr(-24).yb(-68).stat_string(STAT_MINISCORE_SECOND_VAL).endifstat().endifstat();
	sb.ifstat(STAT_MINISCORE_SECOND_POS).xr(-28).yb(-85).pic(STAT_MINISCORE_SECOND_POS).endifstat();
	sb.ifstat(STAT_MINISCORE_FIRST_PIC).xr(-28).yb(-57).stat_string(STAT_SCORELIMIT).endifstat();

	sb.ifstat(STAT_CROSSHAIR_ID_VIEW).xv(122).yb(-128).stat_pname(STAT_CROSSHAIR_ID_VIEW).endifstat();
	sb.ifstat(STAT_CROSSHAIR_ID_VIEW_COLOR).xv(156).yb(-118).pic(STAT_CROSSHAIR_ID_VIEW_COLOR).endifstat();
}

/*
=========================
G_InitStatusbar
=========================
*/
static void G_InitStatusbar() {
	statusbar_t sb;
	bool minhud = g_instaGib->integer || g_nadeFest->integer;

	sb.yb(-24);

	// Health block - compact for minHUD, full otherwise
	sb.ifstat(STAT_SHOW_STATUSBAR)
		.xv(minhud ? 100 : 0).hnum().xv(minhud ? 150 : 50).pic(STAT_HEALTH_ICON)
		.endifstat();

	if (!minhud) AddCombatHUD(sb);
	AddPowerupsAndTech(sb);

	if (CooperativeModeOn() || G_LimitedLivesInLMS()) AddCoopStatus(sb);
	if (!deathmatch->integer) AddSPExtras(sb);
	else AddDeathmatchStatus(sb);

	gi.configString(CS_STATUSBAR, sb.sb.str().c_str());
}

/*
=========================
ApplyMapSettingOverrides
=========================
*/
static void ApplyMapSettingOverrides() {
	// Start with base values from cvars
	game.map.spawnArmor = !g_no_armor->integer;
	game.map.spawnAmmo = true;
	game.map.spawnPowerArmor = !g_no_armor->integer;
	game.map.spawnPowerups = !g_no_powerups->integer;
	game.map.spawnHealth = !g_no_health->integer;
	game.map.spawnBFG = !g_mapspawn_no_bfg->integer;
	game.map.spawnPlasmaBeam = !g_mapspawn_no_plasmabeam->integer;
	game.map.fallingDamage = g_fallingDamage->integer;
	game.map.selfDamage = g_selfDamage->integer;
	game.map.weaponsStay = match_weaponsStay->integer;

	uint16_t enableFlags = game.map.overrideEnableFlags;
	uint16_t disableFlags = game.map.overrideDisableFlags;

	// Apply overrides
	if (enableFlags & MAPFLAG_PU)  game.map.spawnPowerups = true;
	else if (disableFlags & MAPFLAG_PU) game.map.spawnPowerups = false;

	if (enableFlags & MAPFLAG_PA)  game.map.spawnPowerArmor = true;
	else if (disableFlags & MAPFLAG_PA) game.map.spawnPowerArmor = false;

	if (enableFlags & MAPFLAG_AR)  game.map.spawnArmor = true;
	else if (disableFlags & MAPFLAG_AR) game.map.spawnArmor = false;

	if (enableFlags & MAPFLAG_AM)  game.map.spawnAmmo = true;
	else if (disableFlags & MAPFLAG_AM) game.map.spawnAmmo = false;

	if (enableFlags & MAPFLAG_HT)  game.map.spawnHealth = true;
	else if (disableFlags & MAPFLAG_HT) game.map.spawnHealth = false;

	if (enableFlags & MAPFLAG_BFG)  game.map.spawnBFG = true;
	else if (disableFlags & MAPFLAG_BFG) game.map.spawnBFG = false;

	if (enableFlags & MAPFLAG_PB)  game.map.spawnPlasmaBeam = true;
	else if (disableFlags & MAPFLAG_PB) game.map.spawnPlasmaBeam = false;

	if (enableFlags & MAPFLAG_FD)   game.map.fallingDamage = true;
	else if (disableFlags & MAPFLAG_FD)  game.map.fallingDamage = false;

	if (enableFlags & MAPFLAG_SD)   game.map.selfDamage = true;
	else if (disableFlags & MAPFLAG_SD)  game.map.selfDamage = false;

	if (enableFlags & MAPFLAG_WS)   game.map.weaponsStay = true;
	else if (disableFlags & MAPFLAG_WS)  game.map.weaponsStay = false;
}

/*
===================
PickRandomArena
===================
*/
static int32_t PickRandomArena() {
	if (level.arenaTotal <= 0)
		return 1; // default fallback

	return (irandom(level.arenaTotal) + 1); // irandom is 0-based
}

/*
==============
AssignMapLongName

Sanitizes worldspawn "message" for level.longName.
Keeps printable ASCII (including space), skips quotes and slashes,
replaces junk with '-', stops at first linebreak/tab after starting.
==============
*/
static void AssignMapLongName(const gentity_t* ent) {
	const char* fallback = level.mapName.data();
	const char* raw = ent->message;

	if (!raw || !raw[0]) {
		Q_strlcpy(level.longName.data(), fallback, level.longName.size());
		gi.configString(CS_NAME, level.longName.data());
		return;
	}

	std::string clean;
	bool started = false;

	for (const char* s = raw; *s; ++s) {
		unsigned char c = static_cast<unsigned char>(*s);

		// Skip leading junk before we've started
		if (!started && (c < 32 || c >= 127))
			continue;

		// After starting, stop at linebreak/tab
		if (started && (c == '\n' || c == '\r' || c == '\t'))
			break;

		// Skip quotes and backslashes
		if (c == '"' || c == '\\')
			continue;

		// Replace anything outside printable ASCII with '-'
		if (c < 32 || c >= 127) {
			clean += '-';
			started = true;
			continue;
		}

		// Valid printable ASCII (includes space)
		clean += static_cast<char>(c);
		started = true;
	}

	// Collapse multiple spaces
	std::string collapsed;
	bool inSpace = false;
	for (char c : clean) {
		if (c == ' ') {
			if (!inSpace) {
				collapsed += c;
				inSpace = true;
			}
		}
		else {
			collapsed += c;
			inSpace = false;
		}
	}

	// Trim leading/trailing space
	size_t begin = collapsed.find_first_not_of(' ');
	size_t end = collapsed.find_last_not_of(' ');

	const char* src = fallback;
	if (begin != std::string::npos && end != std::string::npos) {
		std::string final = collapsed.substr(begin, end - begin + 1);
		if (!final.empty()) {
			Q_strlcpy(level.longName.data(), final.c_str(), level.longName.size());
			gi.configString(CS_NAME, level.longName.data());
			return;
		}
	}

	// Fallback
	Q_strlcpy(level.longName.data(), fallback, level.longName.size());
	gi.configString(CS_NAME, level.longName.data());
}

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"				environment map name
"skyAxis"			vector axis for rotating sky
"skyRotate"			speed of rotation in degrees/second
"sounds"			music cd track number
"music"				specific music file to play, overrides "sounds"
"gravity"			800 is default gravity
"hub_map"			in campaigns, sets as hub map
"message"			sets long level name
"author"			sets level author name
"author2"			sets another level author name
"start_items"		give players these items on spawn
"no_grapple"		disables grappling hook
"no_dm_spawnpads"	disables spawn pads in deathmatch
"no_dm_telepads"	disables teleporter pads
"ruleset"			overrides gameplay ruleset (q1/q2/q3a)
*/
void SP_worldspawn(gentity_t* ent) {
	std::string name = fmt::format("{} v{}", worr::version::kGameTitle, worr::version::kGameVersion);
	std::strncpy(level.gamemod_name.data(), name.c_str(), level.gamemod_name.size() - 1);
	level.gamemod_name.back() = '\0'; // Ensure null-termination

	ent->moveType = MoveType::Push;
	ent->solid = SOLID_BSP;
	ent->inUse = true; // since the world doesn't use Spawn()
	ent->s.modelIndex = MODELINDEX_WORLD;
	ent->gravity = 1.0f;

	if (st.achievement && st.achievement[0])
		level.achievement = st.achievement;

	ApplyMapSettingOverrides();

	//---------------

	// set configstrings for items
	SetItemNames();

	if (st.nextMap && st.nextMap[0])
		Q_strlcpy(level.nextMap.data(), st.nextMap, level.nextMap.size());

	// make some data visible to the server

	AssignMapLongName(ent);

	if (st.author && st.author[0])
		Q_strlcpy(level.author, st.author, sizeof(level.author));
	if (st.author2 && st.author2[0])
		Q_strlcpy(level.author2, st.author2, sizeof(level.author2));

	if (st.ruleset && st.ruleset[0] && g_level_rulesets->integer) {
		game.ruleset = RS_IndexFromString(st.ruleset);
		if (game.ruleset == Ruleset::None)
			game.ruleset = Ruleset(std::clamp(g_ruleset->integer, 1, static_cast<int>(Ruleset::RS_NUM_RULESETS)));
	}
	else if ((int)game.ruleset != g_ruleset->integer)
		game.ruleset = Ruleset(std::clamp(g_ruleset->integer, 1, static_cast<int>(Ruleset::RS_NUM_RULESETS)));

	if (deathmatch->integer) {
		if (st.arena) {
			level.arenaTotal = st.arena;
			level.arenaActive = PickRandomArena();
		}

		if (Teams() && Game::IsNot(GameType::RedRover))
			gi.configString(CS_GAME_STYLE, G_Fmt("{}", (int32_t)game_style_t::GAME_STYLE_TDM).data());
		else
			gi.configString(CS_GAME_STYLE, G_Fmt("{}", (int32_t)game_style_t::GAME_STYLE_FFA).data());
	}
	else {
		gi.configString(CS_GAME_STYLE, G_Fmt("{}", (int32_t)game_style_t::GAME_STYLE_PVE).data());

		if (st.goals) {
			level.campaign.goals = st.goals;
			game.help[0].modificationCount++;
		}

		if (st.hub_map) {
			level.campaign.hub_map = true;

			// Reset help messages
			game.help[0] = HelpMessage{};
			game.help[1] = HelpMessage{};

			// Reset per-client help tracking
			for (auto ec : active_clients()) {
				auto& cl = *ec->client;
				cl.pers.game_help1changed = 0;
				cl.pers.game_help2changed = 0;
				cl.resp.coopRespawn.game_help1changed = 0;
				cl.resp.coopRespawn.game_help2changed = 0;
			}
		}
	}

	if (st.sky && st.sky[0])
		gi.configString(CS_SKY, st.sky);
	else
		gi.configString(CS_SKY, "unit1_");

	gi.configString(CS_SKYROTATE, G_Fmt("{} {}", st.skyRotate, st.skyAutoRotate).data());

	gi.configString(CS_SKYAXIS, G_Fmt("{}", st.skyAxis).data());

	if (st.music && st.music[0]) {
		gi.configString(CS_CDTRACK, st.music);
	}
	else {
		gi.configString(CS_CDTRACK, G_Fmt("{}", ent->sounds).data());
	}

	if (level.isN64)
		gi.configString(CS_CD_LOOP_COUNT, "0");
	else if (st.was_key_specified("loop_count"))
		gi.configString(CS_CD_LOOP_COUNT, G_Fmt("{}", st.loop_count).data());
	else
		gi.configString(CS_CD_LOOP_COUNT, "");

	if (st.instantItems > 0 || level.isN64)
		level.instantItems = true;

	if (st.start_items)
		level.start_items = st.start_items;

	if (st.no_grapple)
		level.no_grapple = true;

	if (deathmatch->integer && (st.no_dm_spawnpads || level.isN64))
		level.no_dm_spawnpads = true;

	if (deathmatch->integer && st.no_dm_telepads)
		level.no_dm_telepads = true;

	gi.configString(CS_MAXCLIENTS, G_Fmt("{}", game.maxClients).data());

	if (level.isN64 && !deathmatch->integer) {
		gi.configString(CONFIG_N64_PHYSICS_MEDAL, "1");
		pm_config.n64Physics = true;
	}

	// statusbar prog
	G_InitStatusbar();

	// [Paril-KEX] air accel handled by game DLL now, and allow
	// it to be changed in sp/coop
	gi.configString(CS_AIRACCEL, G_Fmt("{}", g_airAccelerate->integer).data());
	pm_config.airAccel = g_airAccelerate->integer;

	game.airAcceleration_modCount = g_airAccelerate->modifiedCount;

	//---------------

	if (!st.gravity) {
		level.gravity = 800.f;
		gi.cvarSet("g_gravity", "800");
	}
	else {
		level.gravity = atof(st.gravity);
		gi.cvarSet("g_gravity", st.gravity);
	}

	snd_fry.assign("player/fry.wav"); // standing in lava / slime

	if (g_dm_random_items->integer) {
		for (item_id_t i = static_cast<item_id_t>(IT_NULL + 1); i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
			PrecacheItem(GetItemByIndex(i));
	}
	else {

		PrecacheItem(GetItemByIndex(IT_COMPASS));

		if (!g_instaGib->integer && !g_nadeFest->integer && Game::IsNot(GameType::ProBall)) {
			switch (game.ruleset) {
			case Ruleset::Quake1:
				PrecacheItem(&itemList[IT_WEAPON_CHAINFIST]);
				PrecacheItem(&itemList[IT_WEAPON_SHOTGUN]);
				PrecacheItem(&itemList[IT_PACK]);
				break;
			case Ruleset::Quake2:
				PrecacheItem(&itemList[IT_WEAPON_BLASTER]);
				break;
			case Ruleset::Quake3Arena:
				PrecacheItem(&itemList[IT_WEAPON_CHAINFIST]);
				PrecacheItem(&itemList[IT_WEAPON_MACHINEGUN]);
				break;
			}
		}

		if (Game::Is(GameType::ProBall))
			PrecacheItem(&itemList[IT_BALL]);

		if ((!strcmp(g_allow_grapple->string, "auto")) ?
			(Game::Has(GameFlags::CTF) ? !level.no_grapple : 0) :
			g_allow_grapple->integer) {
			PrecacheItem(&itemList[IT_WEAPON_GRAPPLE]);
		}
	}

	PrecachePlayerSounds();

	// sexed models
	for (auto& item : itemList)
		item.viewWeaponIndex = 0;

	for (auto& item : itemList) {
		if (!item.viewWeaponModel)
			continue;

		for (auto& check : itemList) {
			if (check.viewWeaponModel && !Q_strcasecmp(item.viewWeaponModel, check.viewWeaponModel) && check.viewWeaponIndex) {
				item.viewWeaponIndex = check.viewWeaponIndex;
				break;
			}
		}

		if (item.viewWeaponIndex)
			continue;

		item.viewWeaponIndex = gi.modelIndex(item.viewWeaponModel);

		if (!level.viewWeaponOffset)
			level.viewWeaponOffset = item.viewWeaponIndex;
	}

	PrecacheAssets();

	// reset heatmap
	HM_ResetForNewLevel();

	//
	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
	//

	// 0 normal
	gi.configString(CS_LIGHTS + 0, "m");

	// 1 FLICKER (first variety)
	gi.configString(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	gi.configString(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	gi.configString(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	gi.configString(CS_LIGHTS + 4, "mamamamamama");

	// 5 GENTLE PULSE 1
	gi.configString(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	gi.configString(CS_LIGHTS + 6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)`map
	gi.configString(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	gi.configString(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	gi.configString(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	gi.configString(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configString(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// [Paril-KEX] 12 N64's 2 (fast strobe)
	gi.configString(CS_LIGHTS + 12, "zzazazzzzazzazazaaazazzza");

	// [Paril-KEX] 13 N64's 3 (half of strong pulse)
	gi.configString(CS_LIGHTS + 13, "abcdefghijklmnopqrstuvwxyz");

	// [Paril-KEX] 14 N64's 4 (fast strobe)
	gi.configString(CS_LIGHTS + 14, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configString(CS_LIGHTS + 63, "a");

	GT_SetLongName();

	// coop respawn strings
	if (CooperativeModeOn()) {
		gi.configString(CONFIG_COOP_RESPAWN_STRING + 0, "$g_coop_respawn_in_combat");
		gi.configString(CONFIG_COOP_RESPAWN_STRING + 1, "$g_coop_respawn_bad_area");
		gi.configString(CONFIG_COOP_RESPAWN_STRING + 2, "$g_coop_respawn_blocked");
		gi.configString(CONFIG_COOP_RESPAWN_STRING + 3, "$g_coop_respawn_waiting");
		gi.configString(CONFIG_COOP_RESPAWN_STRING + 4, "$g_coop_respawn_no_lives");
	}
}
