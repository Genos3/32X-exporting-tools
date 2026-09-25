import os

import bpy
import importlib.util
import math

from bpy_extras.io_utils import ExportHelper
from bpy.app.handlers import persistent
from bpy.props import StringProperty
from mathutils import Vector, Euler

import gpu

from gpu_extras.batch import batch_for_shader


ENTITY_DATABASE = {}
CATEGORY_DATA = {}
ENTITY_CATEGORIES = []

LINK_COLORS = {
    "paths": (0.2, 1.0, 0.2, 1.0), # green
    "triggers": (1.0, 1.0, 0.0, 1.0), # yellow
    "doors": (0.2, 0.9, 1.0, 1.0), # cyan
    "buttons": (1.0, 0.6, 0.2, 1.0), # orange
    "enemies": (1.0, 0.2, 0.2, 1.0), # red
    "default": (1.0, 1.0, 1.0, 1.0), # white
}

ANGLE_COLORS = {
    "enemies": (1.0, 0.2, 0.2, 1.0), # red
    "info": (0.3, 1.0, 0.4, 1.0), # green
    "doors": (0.2, 0.6, 1.0, 1.0), # blue
    "buttons": (1.0, 0.6, 0.2, 1.0), # orange
}

ENABLE_DEPTH_BUFFER = False

last_active_object = None


@persistent
def load_entity_file(dummy):
    scene = bpy.context.scene
    
    if scene:
        load_entity_definitions(scene.entity_definition_file)


def load_entity_definitions(filepath):
    global ENTITY_DATABASE
    global CATEGORY_DATA
    global ENTITY_CATEGORIES
    global LIQUID_TYPES
    
    if not filepath:
        return
    
    filepath = bpy.path.abspath(filepath)
    
    if not os.path.exists(filepath):
        return
    
    spec = importlib.util.spec_from_file_location("quake_entities", filepath)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    
    ENTITY_DATABASE = getattr(module, "ENTITY_DATABASE", {})
    CATEGORY_DATA = getattr(module, "CATEGORY_DATA", {})
    LIQUID_TYPES = getattr(module, "LIQUID_TYPES", {})
    
    ENTITY_CATEGORIES.clear()
    
    categories = list(dict.fromkeys(ENTITY_DATABASE.values()))
    
    for category in categories:
        ENTITY_CATEGORIES.append(
            (category, category.capitalize(), "")
        )


def update_entity_file(self, context):
    filepath = context.scene.entity_definition_file
    load_entity_definitions(filepath)


def get_entity_categories(self, context):
    return ENTITY_CATEGORIES if ENTITY_CATEGORIES else [("NONE", "None", "")]


def get_entity_types(self, context):
    category = context.scene.entity_category
    
    return [
        (name, name, "")
        for name, cat in ENTITY_DATABASE.items()
        if cat == category
    ]


def get_liquid_types():
    return LIQUID_TYPES


def update_show_names(self, context):
    for obj in bpy.data.objects:
        
        if "classname" in obj:
            obj.show_name = context.scene.show_names


def update_active_entity(scene, depsgraph):    
    global last_active_object
    
    obj = bpy.context.object
    
    if obj == last_active_object:
        return
    
    last_active_object = obj
    
    if obj is None:
        return
    
    classname = obj.get("classname")
    
    if classname is None:
        return
    
    scene = bpy.context.scene
    
    category = ENTITY_DATABASE.get(classname, "default")
    
    if category != "default":
        scene.entity_category = category
    
    items = [
        item[0]
        for item in get_entity_types(None, bpy.context)
    ]
    
    if classname in items:
        scene.entity_type = classname
    else:
        scene.entity_type = ""


def draw_arrows():
    if bpy.context.mode != 'OBJECT':
        return
    
    if not bpy.context.scene.show_links:
        return
    
    region_3d = bpy.context.space_data.region_3d
    view_dir = region_3d.view_rotation @ Vector((0.0, 0.0, 1.0))
    
    link_vertices = {}
    angle_vertices = {}
    
    draw_links(link_vertices, view_dir)
    draw_entity_direction(angle_vertices, view_dir)
    
    shader = gpu.shader.from_builtin('UNIFORM_COLOR')
    shader.bind()
    
    gpu.state.depth_test_set(
        'LESS_EQUAL' if ENABLE_DEPTH_BUFFER else 'NONE'
    )
    gpu.state.blend_set('ALPHA')
    gpu.state.line_width_set(2.0)
    
    for category, vertices in link_vertices.items():
        if not vertices:
            continue
        color = LINK_COLORS.get(category, (1.0, 1.0, 1.0, 1.0))
        shader.uniform_float("color", color)
        batch = batch_for_shader(shader, 'LINES', {"pos": vertices})
        batch.draw(shader)
    
    for category, vertices in angle_vertices.items():
        if not vertices:
            continue
        color = ANGLE_COLORS.get(category, (1.0, 1.0, 1.0, 1.0))
        shader.uniform_float("color", color)
        batch = batch_for_shader(shader, 'LINES', {"pos": vertices})
        batch.draw(shader)
    
    gpu.state.depth_test_set('NONE')


def draw_links(link_vertices, view_dir):
    targetnames = {}
    
    for obj in bpy.data.objects:
        
        targetname = obj.get("targetname")
        
        if targetname:
            targetnames[targetname] = obj
    
    for obj in bpy.data.objects:
        if obj.hide_get() or not obj.visible_get():
            continue
        
        for key in (
            "target",
            "killtarget",
            "pathtarget",
            "target2",
            "movewith",
        ):
            target = obj.get(key)
            
            if not target:
                continue
            
            other = targetnames.get(target)
            
            if other is None:
                continue
            
            classname = obj.get("classname")
            
            if classname is None:
                continue
            
            category = ENTITY_DATABASE.get(classname, "default")
            
            vertices = link_vertices.setdefault(category, [])
            
            start = obj.matrix_world.translation
            end = other.matrix_world.translation
            
            vertices.append(start)
            vertices.append(end)
            
            arrow_size = 0.5
            
            direction = end - start
            
            if direction.length > 0.001:
                
                direction.normalize()
                
                arrow_base = end - direction * arrow_size
                
                side = direction.cross(view_dir)
                
                if side.length < 0.001:
                    side = Vector((1.0, 0.0, 0.0))
                
                side.normalize()
                
                wing = arrow_size * 0.35
                
                left = arrow_base + side * wing
                right = arrow_base - side * wing
                
                vertices.append(end)
                vertices.append(left)
                
                vertices.append(end)
                vertices.append(right)


def draw_entity_direction(angle_vertices, view_dir):
    for obj in bpy.data.objects:
        if obj.hide_get() or not obj.visible_get():
            continue
        
        classname = obj.get("classname")
        if classname is None:
            continue
        
        category = ENTITY_DATABASE.get(classname, "default")
        
        if category not in ANGLE_COLORS:
            continue
        
        direction = None
        
        if "angle" in obj:
            angle = float(obj["angle"])
            if angle == -1:
                direction = Vector((0.0, 0.0, 1.0))
            elif angle == -2:
                direction = Vector((0.0, 0.0, -1.0))
            else:
                radians = math.radians(angle)
                direction = Vector((math.cos(radians), math.sin(radians), 0.0))
        
        elif "mangle" in obj:
            pitch, yaw, roll = (float(v) for v in str(obj["mangle"]).split())
            direction = (
                Euler((math.radians(pitch), 0.0, math.radians(yaw)), 'XYZ').to_matrix()
                @ Vector((1.0, 0.0, 0.0))
            )
            
            print(f"{obj.name} direction:", direction)
        
        if direction is None:
            continue
        
        vertices = angle_vertices.setdefault(category, [])
        
        start = obj.matrix_world.translation
        end = start + direction.normalized() * 0.75
        
        vertices.append(start)
        vertices.append(end)
        
        arrow_size = 0.2
        arrow_base = end - direction.normalized() * arrow_size
        
        side = direction.cross(view_dir)
        if side.length < 0.001:
            side = direction.cross(Vector((0.0, 0.0, 1.0)))
        side.normalize()
        
        wing = arrow_size * 0.5
        
        vertices.append(end)
        vertices.append(arrow_base + side * wing)
        vertices.append(end)
        vertices.append(arrow_base - side * wing)


class entity_properties(bpy.types.PropertyGroup):
    sector: bpy.props.PointerProperty(
        name="Sector",
        type=bpy.types.Object,
        description="The sector this entity belongs to"
    )


class OBJECT_OT_assign_entity(bpy.types.Operator):
    bl_idname = "object.assign_entity"
    bl_label = "Assign to Selected"
    
    def execute(self, context):
        entity_type = context.scene.entity_type
        
        if not entity_type:
            return {'FINISHED'}
        
        for obj in context.selected_objects:
            # Get the new category
            new_category = ENTITY_DATABASE.get(entity_type)
            if not new_category:
                continue
            
            # Get the data for this category
            new_cat_data = CATEGORY_DATA.get(new_category, {})
            expected_object_type = new_cat_data.get("object_type", "EMPTY")
            
            # Check if object type matches (MESH vs EMPTY vs LIGHT)
            if obj.type != expected_object_type:
                continue
            
            old_type = obj.get("classname", "")
            old_category = ENTITY_DATABASE.get(old_type)
            
            old_cat_data = CATEGORY_DATA.get(old_category, {})
            old_properties = old_cat_data.get("properties", set())
            
            new_properties = new_cat_data.get("properties", set())
            
            obj["classname"] = entity_type
            
            for key in list(obj.keys()):
                if key in ("_RNA_UI", "classname"):
                    continue
                
                if (
                    key in old_properties and
                    key not in new_properties
                ):
                    del obj[key]
        
        return {'FINISHED'}


class OBJECT_OT_export_entities(
    bpy.types.Operator,
    ExportHelper
):
    bl_idname = "object.export_entities"
    bl_label = "Export Entities"
    
    filename_ext = ".ent"
    
    filter_glob: StringProperty(
        default="*.ent",
        options={'HIDDEN'}
    )
    
    def execute(self, context):
        
        with open(self.filepath, "w") as file:
            
            for obj in bpy.data.objects:
                
                if "classname" not in obj:
                    continue
                
                file.write("entity\n")
                file.write("{\n")
                
                for key in sorted(obj.keys()):
                    
                    if key == "_RNA_UI":
                        continue
                    
                    file.write(
                        f"{key} {obj[key]}\n"
                    )
                
                x = obj.location.x
                y = obj.location.y
                z = obj.location.z
                
                file.write(
                    f"origin {x} {y} {z}\n"
                )
                
                file.write("}\n\n")
        
        return {'FINISHED'}


class VIEW3D_PT_entity_panel(bpy.types.Panel):
    bl_label = "Entity"
    bl_idname = "VIEW3D_PT_entity_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Entities"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object
        
        layout.use_property_split = True
        layout.use_property_decorate = False
        
        classname = obj.get("classname") if obj else None
        
        layout.label(text="Entity Definition")
        
        layout.prop(context.scene, "entity_definition_file", text="")
        
        layout.separator()
        
        layout.prop(context.scene, "entity_category")
        layout.prop(context.scene, "entity_type")
        
        layout.use_property_split = False
        
        layout.operator("object.assign_entity")
        
        layout.separator()
        
        layout.prop(context.scene, "show_links")
        layout.prop(context.scene, "show_names")
        
        layout.separator()
        
        layout.operator("object.export_entities")
        
        if classname is None:
            layout.label(text="No entity selected")
            return
        
        layout.label(text=f"Classname: {classname}")
        layout.use_property_split = True
        
        box = layout.box()
        box.label(text="Properties")
        
        for key in sorted(obj.keys()):
            if key in ("_RNA_UI", "map_props", "entity_props"):
                continue
            
            box.prop(
                obj,
                f'["{key}"]',
                text=key
            )


classes = (
    entity_properties,
    VIEW3D_PT_entity_panel,
    OBJECT_OT_assign_entity,
    OBJECT_OT_export_entities,
)


draw_handler = None


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Object.entity_props = bpy.props.PointerProperty(
        type=entity_properties
    )
    
    bpy.app.handlers.load_post.append(
        load_entity_file
    )
    
    bpy.types.Scene.entity_definition_file = bpy.props.StringProperty(
        name="Entity Definition",
        subtype='FILE_PATH',
        update=update_entity_file
    )
    
    bpy.types.Scene.entity_category = bpy.props.EnumProperty(
        name="Category",
        items=get_entity_categories
    )
    
    bpy.types.Scene.entity_type = bpy.props.EnumProperty(
        name="Type",
        items=get_entity_types
    )
    
    bpy.types.Scene.show_links = bpy.props.BoolProperty(
        name="Show Links",
        default=True
    )
    
    bpy.types.Scene.show_names = bpy.props.BoolProperty(
        name="Show Names",
        default=True,
        update=update_show_names
    )
    
    global draw_handler 
    
    draw_handler = bpy.types.SpaceView3D.draw_handler_add(
        draw_arrows,
        (),
        'WINDOW',
        'POST_VIEW'
    )
    
    bpy.app.handlers.depsgraph_update_post.append(
        update_active_entity
    )


def unregister():
    del bpy.types.Object.entity_props
    del bpy.types.Scene.entity_definition_file
    del bpy.types.Scene.entity_category
    del bpy.types.Scene.entity_type
    del bpy.types.Scene.show_links
    del bpy.types.Scene.show_names
    
    if load_entity_file in bpy.app.handlers.load_post:
        bpy.app.handlers.load_post.remove(
            load_entity_file
        )
    
    if update_active_entity in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.remove(
            update_active_entity
        )
    
    global draw_handler
    
    if draw_handler is not None:
        bpy.types.SpaceView3D.draw_handler_remove(
            draw_handler,
            'WINDOW'
        )
        
        draw_handler = None
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)