bl_info = {
    "name": "Map Tools",
    "blender": (3, 5, 0),
    "location": "View3D > Sidebar",
    "description": "Map panel, face panel, pvs panel, sector panel, entity panel",
    "category": "3D View",
}

if "bpy" in locals():
    import importlib
    
    if "map_panel" in locals():
        importlib.reload(map_panel)
    if "face_panel" in locals():
        importlib.reload(face_panel)
    if "entity_panel" in locals():
        importlib.reload(entity_panel)
    if "pvs_panel" in locals():
        importlib.reload(pvs_panel)
    if "sector_panel" in locals():
        importlib.reload(sector_panel)

import bpy

from . import map_panel
from . import face_panel
from . import entity_panel
from . import pvs_panel
from . import sector_panel


def register():
    map_panel.register()
    face_panel.register()
    entity_panel.register()
    pvs_panel.register()
    sector_panel.register()


def unregister():
    map_panel.unregister()
    face_panel.unregister()
    entity_panel.unregister()
    pvs_panel.unregister()
    sector_panel.unregister()


if __name__ == "__main__":
    register()