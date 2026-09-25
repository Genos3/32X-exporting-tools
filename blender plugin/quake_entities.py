ENTITY_DATABASE = {
    # Enemies
    "monster_army": "enemies",
    "monster_boss": "enemies",
    "monster_demon1": "enemies",
    "monster_dog": "enemies",
    "monster_enforcer": "enemies",
    "monster_hell_knight": "enemies",
    "monster_knight": "enemies",
    "monster_ogre": "enemies",
    "monster_oldone": "enemies",
    "monster_shambler": "enemies",
    "monster_tarbaby": "enemies",
    "monster_vore": "enemies",
    "monster_wizard": "enemies",
    "monster_zombie": "enemies",
    
    # Items
    "item_armor1": "items",
    "item_armor2": "items",
    "item_armorInv": "items",
    "item_artifact_envirosuit": "items",
    "item_artifact_invisibility": "items",
    "item_artifact_invulnerability": "items",
    "item_artifact_super_damage": "items",
    "item_cells": "items",
    "item_health": "items",
    "item_key1": "items",
    "item_key2": "items",
    "item_rockets": "items",
    "item_shells": "items",
    "item_sigil": "items",
    "item_spikes": "items",
    
    # Weapons
    "weapon_grenadelauncher": "weapons",
    "weapon_lightning": "weapons",
    "weapon_nailgun": "weapons",
    "weapon_rocketlauncher": "weapons",
    "weapon_shotgun": "weapons",
    "weapon_supernailgun": "weapons",
    "weapon_supershotgun": "weapons",
    
    # Info
    "info_intermission": "info",
    "info_notnull": "info",
    "info_null": "info",
    "info_player_coop": "info",
    "info_player_deathmatch": "info",
    "info_player_start": "info",
    "info_player_start2": "info",
    "info_teleport_destination": "info",
    
    # Paths
    "path_corner": "paths",
    
    # Misc
    "misc_explobox": "misc",
    "misc_explobox2": "misc",
    "misc_fireball": "misc",
    "misc_noisemaker": "misc",
    "misc_teleporttrain": "misc",
    
    # Triggers
    "trigger_changelevel": "triggers",
    "trigger_counter": "triggers",
    "trigger_hurt": "triggers",
    "trigger_monsterjump": "triggers",
    "trigger_multiple": "triggers",
    "trigger_once": "triggers",
    "trigger_onlyregistered": "triggers",
    "trigger_push": "triggers",
    "trigger_relay": "triggers",
    "trigger_secret": "triggers",
    "trigger_teleport": "triggers",
    
    # Doors
    "func_door": "doors",
    "func_door_secret": "doors",
    
    # Buttons
    "func_button": "buttons",
    
    # Platforms & Brushes
    "func_bossgate": "platforms",
    "func_episodegate": "platforms",
    "func_illusionary": "platforms",
    "func_plat": "platforms",
    "func_train": "platforms",
    "func_wall": "platforms",
    
    # Ambient Sounds
    "ambient_comp_hum": "ambient",
    "ambient_drip": "ambient",
    "ambient_drone": "ambient",
    "ambient_flock_gulls": "ambient",
    "ambient_light_buzz": "ambient",
    "ambient_suck_wind": "ambient",
    "ambient_swamp1": "ambient",
    "ambient_swamp2": "ambient",
    "ambient_thunder": "ambient",
    
    # Lights
    "light": "lights",
    "light_environment": "lights",
    "light_flame_large_yellow": "lights",
    "light_flame_small_white": "lights",
    "light_flame_small_yellow": "lights",
    "light_fluoro": "lights",
    "light_fluorospark": "lights",
    "light_globe": "lights",
    "light_torch_small_wallable": "lights",
}

LIQUID_TYPES = {
    "water": {
        "sector_type": "WATER",
        "is_semitransparent": True,
    },
    "slime": {
        "sector_type": "SLIME",
        "is_semitransparent": True,
    },
    "acid": {
        "sector_type": "ACID",
        "is_semitransparent": True,
    },
    "lava": {
        "sector_type": "LAVA",
        "is_semitransparent": False,
    },
}

CATEGORY_DATA = {
    "enemies": {
        "object_type": "EMPTY",
        "properties": {
            "angle",
            "target",
            "targetname",
            "killtarget"
        },
    },
    
    "items": {
        "object_type": "EMPTY",
        "properties": {
            "target",
            "targetname",
            "killtarget"
        },
    },
    
    "weapons": {
        "object_type": "EMPTY",
        "properties": {
            "target",
            "targetname",
            "killtarget"
        },
    },
    
    "info": {
        "object_type": "EMPTY",
        "properties": {
            "angle",
            "mangle",
            "targetname"
        },
    },
    
    "paths": {
        "object_type": "EMPTY",
        "properties": {
            "target",
            "targetname"
        },
    },
    
    "misc": {
        "object_type": "EMPTY",
        "properties": {
            "angle",
            "target",
            "targetname"
        },
    },
    
    "triggers": {
        "object_type": "MESH",
        "properties": {
            "target",
            "targetname",
            "killtarget",
            "message",
            "wait"
        },
    },
    
    "doors": {
        "object_type": "MESH",
        "properties": {
            "angle",
            "targetname",
            "speed",
            "wait",
            "lip",
            "sounds",
            "health",
            "message"
        },
    },
    
    "buttons": {
        "object_type": "MESH",
        "properties": {
            "angle",
            "target",
            "targetname",
            "speed",
            "wait",
            "lip",
            "health"
        },
    },
    
    "platforms": {
        "object_type": "MESH",
        "properties": {
            "targetname",
            "speed",
            "height"
        },
    },
    
    "ambient": {
        "object_type": "EMPTY",
        "properties": {
            "targetname"
        },
    },
    
    "lights": {
        "object_type": "LIGHT",
        "properties": {
            "light",
            "style",
            "targetname",
            "target",
            "spawnflags",
            "angle",
            "mangle",
        },
    },
}
