bl_info = {
    "name": "Quake BSP Importer",
    "blender": (3, 5, 0),
    "location": "File > Import",
    "description": "Import Quake 1 BSP maps",
    "category": "Import-Export",
}

import bpy
from mathutils import Matrix

from bpy.props import (
    BoolProperty,
    FloatProperty,
    StringProperty,
)

from bpy_extras.io_utils import ImportHelper

from . import quake_bsp_import


class IMPORT_SCENE_OT_quake_bsp(
    bpy.types.Operator,
    ImportHelper
):
    bl_idname = "import_scene.quake_bsp"
    bl_label = "Import Quake BSP"
    
    filename_ext = ".bsp"
    
    filter_glob: StringProperty(
        default="*.bsp",
        options={'HIDDEN'}
    )
    
    scale: FloatProperty(
        name="Scale",
        default=0.03125 # 1 Meter = 32 Quake units
    )
    
    import_world: BoolProperty(
        name="World",
        default=True
    )
    
    import_lights: BoolProperty(
        name="Lights",
        default=True
    )
    
    import_cameras: BoolProperty(
        name="Cameras",
        default=True
    )
    
    import_entities: BoolProperty(
        name="Entities",
        default=True
    )
    
    create_materials: BoolProperty(
        name="Create Materials",
        default=True
    )
    
    extract_textures: BoolProperty(
        name="Extract Textures",
        default=True
    )
    
    def execute(self, context):
        global_matrix = Matrix.Scale(self.scale, 4)
        
        return quake_bsp_import.import_bsp(
            self,
            context,
            self.filepath,
            global_matrix,
            self.import_world,
            self.import_lights,
            self.import_cameras,
            self.import_entities,
            self.create_materials,
            self.extract_textures
        )


def menu_import(self, context):
    self.layout.operator(
        IMPORT_SCENE_OT_quake_bsp.bl_idname,
        text="Quake BSP (.bsp)"
    )


classes = (
    IMPORT_SCENE_OT_quake_bsp,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.TOPBAR_MT_file_import.append(
        menu_import
    )


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(
        menu_import
    )
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()