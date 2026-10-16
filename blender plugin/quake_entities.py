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

ENTITY_COLLISION_BOXES = {
    # Enemies
    "monster_army": ((-16, -16, -24), (16, 16, 40)),
    "monster_dog": ((-32, -32, -24), (32, 32, 40)),
    "monster_ogre": ((-32, -32, -24), (32, 32, 64)),
    "monster_ogre_marksman": ((-32, -32, -24), (32, 32, 64)),
    "monster_knight": ((-16, -16, -24), (16, 16, 40)),
    "monster_hell_knight": ((-16, -16, -24), (16, 16, 40)),
    "monster_wizard": ((-16, -16, -24), (16, 16, 40)),
    "monster_demon1": ((-32, -32, -24), (32, 32, 64)),
    "monster_shambler": ((-32, -32, -24), (32, 32, 64)),
    "monster_boss": ((-128, -128, -24), (128, 128, 256)),
    "monster_enforcer": ((-16, -16, -24), (16, 16, 40)),
    "monster_shalrath": ((-32, -32, -24), (32, 32, 48)),
    "monster_tarbaby": ((-16, -16, -24), (16, 16, 24)),
    "monster_fish": ((-16, -16, -24), (16, 16, 24)),
    "monster_oldone": ((-16, -16, -24), (16, 16, 32)),
    "monster_zombie": ((-16, -16, -24), (16, 16, 32)),

    # Players / Starts
    "info_player_start": ((-16, -16, -24), (16, 16, 32)),
    "info_player_deathmatch": ((-16, -16, -24), (16, 16, 32)),
    "info_player_coop": ((-16, -16, -24), (16, 16, 32)),
    "info_player_start2": ((-16, -16, -24), (16, 16, 32)),
    "info_teleport_destination": ((-16, -16, -24), (16, 16, 32)),
    "info_intermission": ((-16, -16, -24), (16, 16, 32)),
    "path_corner": ((-8, -8, -8), (8, 8, 8)),

    # Items
    "item_cells": ((-16, -16, -16), (16, 16, 16)),
    "item_rockets": ((-16, -16, -16), (16, 16, 16)),
    "item_shells": ((-16, -16, -16), (16, 16, 16)),
    "item_spikes": ((-16, -16, -16), (16, 16, 16)),
    "item_health": ((-16, -16, 0), (16, 16, 16)),
    "item_armor1": ((-16, -16, -24), (16, 16, 32)),
    "item_armor2": ((-16, -16, -24), (16, 16, 32)),
    "item_armorInv": ((-16, -16, -24), (16, 16, 32)),
    "item_key1": ((-16, -16, -24), (16, 16, 32)),
    "item_key2": ((-16, -16, -24), (16, 16, 32)),
    "item_sigil": ((-16, -16, -24), (16, 16, 32)),

    # Weapons
    "weapon_shotgun": ((-16, -16, 0), (16, 16, 32)),
    "weapon_supershotgun": ((-16, -16, 0), (16, 16, 32)),
    "weapon_nailgun": ((-16, -16, 0), (16, 16, 32)),
    "weapon_supernailgun": ((-16, -16, 0), (16, 16, 32)),
    "weapon_grenadelauncher": ((-16, -16, 0), (16, 16, 32)),
    "weapon_rocketlauncher": ((-16, -16, 0), (16, 16, 32)),
    "weapon_lightning": ((-16, -16, 0), (16, 16, 32)),
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

SPAWNFLAGS = {
    "light": {
        1: "Start Off",
    },
    
    "func_door": {
        1: "Start Open",
        2: "Don't Link",
        4: "Gold Key",
        8: "Silver Key",
        16: "Toggle",
        32: "Touch Opens",
    },
    
    "func_door_secret": {
		1 : "Open once only",
		2 : "Moves left first",
		4 : "Moves down first",
		8 : "Not shootable",
        16 : "Always shootable",
    },
    
    "func_button": {
        1: "Don't Link",
    },
    
    "item_health": {
        1: "Rotten",
        2: "Megahealth",
    },
    
    "monster_*": {
        1: "Ambush",
    },
}

GLOBAL_SPAWNFLAGS = {
    256: "Not in Easy",
    512: "Not in Normal",
    1024: "Not in Hard",
    2048: "Not in Deathmatch",
}

LIGHT_STYLES = {
    0: "Normal",
    10: "Fluorescent flicker",
    2: "Slow, strong pulse",
    11: "Slow pulse, noblack",
    5: "Gentle pulse",
    1: "Flicker A",
    6: "Flicker B",
    3: "Candle A",
    7: "Candle B",
    8: "Candle C",
    4: "Fast strobe",
    9: "Slow strobe",
}
