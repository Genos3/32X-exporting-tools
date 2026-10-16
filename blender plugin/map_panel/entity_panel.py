import os

import bpy
import importlib.util
import math

from bpy_extras.io_utils import ExportHelper
from bpy.app.handlers import persistent
from bpy.props import StringProperty
from mathutils import Vector, Euler, Matrix

import gpu

from gpu_extras.batch import batch_for_shader
from bpy_extras.view3d_utils import location_3d_to_region_2d

import blf


ENTITY_DATABASE = {}
CATEGORY_DATA = {}
ENTITY_CATEGORIES = []
GLOBAL_SPAWNFLAGS = {}
SPAWNFLAGS = {}
LIGHT_STYLES = {}
ENTITY_COLLISION_BOXES = {}

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

UNIT_SCALE = 1 / 32

ENABLE_LINKS_DEPTH_TEST = False
DEBUG_DIRECTION = False

last_active_object = None

entity_name_textures = {}


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
    global GLOBAL_SPAWNFLAGS
    global SPAWNFLAGS
    global LIGHT_STYLES
    global ENTITY_COLLISION_BOXES
    
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
    GLOBAL_SPAWNFLAGS = getattr(module, "GLOBAL_SPAWNFLAGS", {})
    SPAWNFLAGS = getattr(module, "SPAWNFLAGS", {})
    LIGHT_STYLES = getattr(module, "LIGHT_STYLES", {})
    ENTITY_COLLISION_BOXES = getattr(module, "ENTITY_COLLISION_BOXES", {})
    
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


def get_entity_spawnflags(classname):
    spawnflags = {}
    
    if classname in SPAWNFLAGS:
        spawnflags.update(SPAWNFLAGS[classname])
    else:
        for entity_type, flags in SPAWNFLAGS.items():
            if entity_type.endswith("*") and classname.startswith(entity_type[:-1]):
                spawnflags.update(flags)
                break
    
    spawnflags.update(GLOBAL_SPAWNFLAGS)
    
    return spawnflags


FLAG_BITS = [1 << i for i in range(16)]


def make_flag_getter(bit):
    def getter(self):
        return bool(int(self.get("spawnflags", 0)) & bit)
    
    return getter


def make_flag_setter(bit):
    def setter(self, value):
        spawnflags = int(self.get("spawnflags", 0))
        if value:
            spawnflags |= bit
        else:
            spawnflags &= ~bit
        self["spawnflags"] = spawnflags
    
    return setter


def get_light_style_items(self, context):
    return [
        (str(style), label, "", style)
        for style, label in LIGHT_STYLES.items()
    ]


def get_light_style(self):
    return int(self.get("style", 0))


def set_light_style(self, value):
    self["style"] = int(value)


def get_liquid_types():
    return LIQUID_TYPES


def poll_sector(self, obj):
    return obj.map_props.map_type == 'SECTOR'


def update_show_entities(self, context):
    for obj in context.scene.objects:
        if obj.map_props.map_type == 'ENTITY':
            obj.hide_set(not self.show_entities)


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


def get_entity_name_texture(name):
    texture = entity_name_textures.get(name)
    
    if texture is not None:
        return texture
    
    # Calculate target size by applying UI scale and DPI ratio (default base is 72)
    prefs = bpy.context.preferences
    ui_scale = prefs.view.ui_scale
    dpi = prefs.system.dpi
    scaled_size = int(12 * ui_scale * (dpi / 72.0))
    
    blf.size(0, scaled_size)
    
    width, height = blf.dimensions(0, name)
    
    width = max(1, int(width) + 4)
    height = max(1, int(height) + 4)
    
    offscreen = gpu.types.GPUOffScreen(width, height)
    
    with offscreen.bind():
        # Clear the background to transparent
        fb = gpu.state.active_framebuffer_get()
        fb.clear(color=(0.0, 0.0, 0.0, 0.0), depth=1.0)
        
        with gpu.matrix.push_pop():
            # Reset the modelview matrix
            gpu.matrix.load_matrix(Matrix.Identity(4))
            
            # Build and load a 2D orthographic projection matrix mapping (0, width) and (0, height)
            proj = Matrix.Identity(4)
            proj[0][0] = 2.0 / width
            proj[1][1] = 2.0 / height
            proj[0][3] = -1.0
            proj[1][3] = -1.0
            gpu.matrix.load_projection_matrix(proj)
            
            # Draw the text in 2D pixel space
            gpu.state.blend_set('ALPHA')
            blf.position(0, 2, 3, 0)
            
            blf.color(0, 1.0, 1.0, 1.0, 1.0)
            
            blf.draw(0, name)
    
    texture = offscreen.texture_color
    
    entity_name_textures[name] = (
        texture,
        width,
        height,
        offscreen
    )
    
    return entity_name_textures[name]


custom_nearest_shader = None

def get_nearest_shader():
    global custom_nearest_shader
    if custom_nearest_shader is None:
        info = gpu.types.GPUShaderCreateInfo()
        info.push_constant('MAT4', "mvp")
        info.push_constant('VEC4', "text_color")
        info.vertex_in(0, 'VEC3', "pos")
        info.vertex_in(1, 'VEC2', "texCoord")
        
        interface = gpu.types.GPUStageInterfaceInfo("shader_interface")
        interface.smooth('VEC2', "uv")
        info.vertex_out(interface)
        
        info.fragment_out(0, 'VEC4', "fragColor")
        info.sampler(0, 'FLOAT_2D', "image")
        
        info.vertex_source(
            "void main() {"
            "  uv = texCoord;"
            "  gl_Position = mvp * vec4(pos, 1.0);"
            "}"
        )
        
        info.fragment_source(
            "void main() {"
            "  ivec2 tex_coord = ivec2(uv * vec2(textureSize(image, 0)));"
            "  vec4 col = texelFetch(image, tex_coord, 0);"
            "  if (col.a < 0.05) discard;"
            "  fragColor = vec4(text_color.rgb, col.a * text_color.a);"
            "}"
        )
        
        custom_nearest_shader = gpu.shader.create_from_info(info)
        
    return custom_nearest_shader


def draw_entity_overlay():
    if bpy.context.mode != 'OBJECT':
        return
    
    # Draw the collision boxes
    
    if bpy.context.scene.show_collision_boxes:
        active_vertices = []
        selected_vertices = []
        unselected_vertices = []
        
        for obj in bpy.data.objects:
            if obj.hide_get() or not obj.visible_get():
                continue
            
            bounds = ENTITY_COLLISION_BOXES.get(obj.get("classname"))
            if bounds is None:
                continue
            
            minimum, maximum = bounds
            x0, y0, z0 = minimum
            x1, y1, z1 = maximum
            
            corners = [
                obj.matrix_world.translation + Vector((x, y, z)) * UNIT_SCALE
                for x, y, z in (
                    (x0, y0, z0), (x1, y0, z0),
                    (x1, y1, z0), (x0, y1, z0),
                    (x0, y0, z1), (x1, y0, z1),
                    (x1, y1, z1), (x0, y1, z1),
                )
            ]
            
            if obj.select_get():
                if obj == bpy.context.view_layer.objects.active:
                    target_vertices = active_vertices
                else:
                    target_vertices = selected_vertices
            else:
                target_vertices = unselected_vertices
            
            for start, end in (
                (0, 1), (1, 2), (2, 3), (3, 0),
                (4, 5), (5, 6), (6, 7), (7, 4),
                (0, 4), (1, 5), (2, 6), (3, 7),
            ):
                target_vertices.extend((corners[start], corners[end]))
        
        if selected_vertices or unselected_vertices:
            shader = gpu.shader.from_builtin('UNIFORM_COLOR')
            gpu.state.depth_test_set('LESS_EQUAL')
            gpu.state.blend_set('ALPHA')
            theme = bpy.context.preferences.themes[0].view_3d
            
            for vertices, color in (
                (unselected_vertices, (0.0, 0.0, 0.0, 0.9)),
                (selected_vertices, (*theme.object_selected, 1.0)),
                (active_vertices, (*theme.object_active, 1.0)),
            ):
                if not vertices:
                    continue
                
                shader.bind()
                shader.uniform_float("color", color)
                batch_for_shader(shader, 'LINES', {"pos": vertices}).draw(shader)
            
            gpu.state.depth_test_set('NONE')
            gpu.state.blend_set('NONE')
    
    # Draw the arrows
    
    if bpy.context.scene.show_links:
        region_3d = bpy.context.space_data.region_3d
        view_dir = region_3d.view_rotation @ Vector((0.0, 0.0, 1.0))
        
        link_vertices = {}
        angle_vertices = {}
        
        draw_links(link_vertices, view_dir)
        draw_entity_direction(angle_vertices, view_dir)
        
        shader = gpu.shader.from_builtin('UNIFORM_COLOR')
        shader.bind()
        
        gpu.state.depth_test_set(
            'LESS_EQUAL' if ENABLE_LINKS_DEPTH_TEST else 'NONE'
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
    
    # Draw the names
    
    if bpy.context.scene.show_names:
        region_3d = bpy.context.space_data.region_3d
        
        right = region_3d.view_rotation @ Vector((1.0, 0.0, 0.0))
        up = region_3d.view_rotation @ Vector((0.0, 1.0, 0.0))
        
        # shader = gpu.shader.from_builtin('IMAGE')
        shader = get_nearest_shader()
        shader.bind()
        
        gpu.state.depth_test_set('LESS_EQUAL')
        gpu.state.blend_set('ALPHA')
        
        # Fetch and multiply matrices manually for Blender 4.0+
        proj_matrix = gpu.matrix.get_projection_matrix()
        view_matrix = gpu.matrix.get_model_view_matrix()
        mvp = proj_matrix @ view_matrix
        
        shader.uniform_float("mvp", mvp)
        
        for obj in bpy.data.objects:
            if obj.hide_get() or not obj.visible_get():
                continue
            
            if "classname" not in obj:
                continue
            
            texture, width, height, offscreen = get_entity_name_texture(obj.name)
            
            is_light = obj.get("classname", "").startswith("light")
            theme = bpy.context.preferences.themes[0].view_3d
            
            if obj.select_get():
                if obj == bpy.context.view_layer.objects.active:
                    text_color = (*theme.object_active, 1.0)
                else:
                    text_color = (*theme.object_selected, 1.0)
            elif is_light:
                text_color = (0.0, 0.0, 0.0, 0.3)
            else:
                text_color = (0.0, 0.0, 0.0, 1.0)
            
            shader.uniform_float("text_color", text_color)
            
            origin = obj.matrix_world.translation
            
            region = bpy.context.region
            
            screen_position = location_3d_to_region_2d(
                region,
                region_3d,
                origin
            )
            
            screen_position_x = location_3d_to_region_2d(
                region,
                region_3d,
                origin + right
            )
            
            # Abort drawing this specific entity if it is behind the camera
            if screen_position is None or screen_position_x is None:
                continue
            
            pixels_per_world_unit = (screen_position_x - screen_position).length
            
            # Prevent DivisionByZero if the object is exactly at a singularity
            if pixels_per_world_unit == 0:
                continue
            
            scale = 1.0 / pixels_per_world_unit
            
            pixel_offset_x = 8.0
            pixel_offset_y = 8.0
            position = origin + up * (pixel_offset_y * scale) + right * (pixel_offset_x * scale)
            
            full_width = width * scale
            half_height = height * scale * 0.5
            
            vertices = [
                position - up * half_height,                        # Bottom-Left
                position + right * full_width - up * half_height,   # Bottom-Right
                position + right * full_width + up * half_height,   # Top-Right
                position + up * half_height,                        # Top-Left
            ]
            
            batch = batch_for_shader(
                shader,
                'TRIS',
                {
                    "pos": [
                        vertices[0], vertices[1], vertices[2],
                        vertices[0], vertices[2], vertices[3],
                    ],
                    "texCoord": [
                        (0.0, 0.0), (1.0, 0.0), (1.0, 1.0),
                        (0.0, 0.0), (1.0, 1.0), (0.0, 1.0),
                    ],
                },
            )
            
            shader.uniform_sampler("image", texture)
            batch.draw(shader)
        
        gpu.state.depth_test_set('NONE')
        gpu.state.blend_set('NONE')


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
            
            if DEBUG_DIRECTION:
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
        poll=poll_sector,
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


class OBJECT_OT_export_entities(bpy.types.Operator):
    bl_idname = "object.export_entities"
    bl_label = "Export Entities"
    
    def execute(self, context):
        
        filepath = os.path.splitext(bpy.data.filepath)[0] + ".ent"
        
        with open(filepath, "w") as file:
            
            for obj in bpy.data.objects:
                
                if "classname" not in obj:
                    continue
                
                file.write("entity\n")
                file.write("{\n")
                
                for key in sorted(obj.keys()):
                    
                    if key in ("_RNA_UI", "map_props", "entity_props"):
                        continue
                    
                    file.write(f"{key} {obj[key]}\n")
                
                if obj.entity_props.sector:
                    file.write(f"sector {obj.entity_props.sector.name}\n")
                
                x = int(obj.location.x * 32)
                y = int(obj.location.y * 32)
                z = int(obj.location.z * 32)
                
                file.write(f"origin {x} {y} {z}\n")
                file.write("}\n\n")
        
        self.report({'INFO'}, f"ent file exported to {filepath}")
        return {'FINISHED'}


class OBJECT_OT_assign_selected_sector(bpy.types.Operator):
    bl_idname = "object.assign_selected_sector"
    bl_label = "Assign Selected"
    bl_description = "Assign the selected sector to all selected entities"
    
    def execute(self, context):
        active_object = context.active_object
        
        if not active_object or not active_object.entity_props.sector:
            self.report({'WARNING'}, "No sector selected")
            return {'CANCELLED'}
        
        sector = active_object.entity_props.sector
        
        for obj in context.selected_objects:
            if obj.map_props.map_type == 'ENTITY':
                obj.entity_props.sector = sector
        
        return {'FINISHED'}


class OBJECT_OT_toggle_spawnflag(bpy.types.Operator):
    bl_idname = "object.toggle_spawnflag"
    bl_label = "Toggle Spawnflag"
    
    flag: bpy.props.IntProperty()
    
    def execute(self, context):
        obj = context.object
        
        if obj is None:
            return {'CANCELLED'}
        
        spawnflags = int(obj.get("spawnflags", 0))
        
        if spawnflags & self.flag:
            spawnflags &= ~self.flag
        else:
            spawnflags |= self.flag
        
        obj["spawnflags"] = spawnflags
        
        return {'FINISHED'}


class OBJECT_OT_toggle_entities(bpy.types.Operator):
    bl_idname = "object.toggle_entities"
    bl_label = "Hide Entities"
    bl_description = "Hide or show all entity objects"
    
    def execute(self, context):
        entities = [
            obj for obj in bpy.context.scene.objects
            if obj.map_props.map_type == 'ENTITY'
        ]
        
        hide_entities = any(not obj.hide_viewport for obj in entities)
        
        for obj in entities:
            obj.hide_set(hide_entities)
        
        self.bl_label = "Show Entities" if hide_entities else "Hide Entities"
        
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
        layout.prop(context.scene, "show_collision_boxes")
        layout.prop(context.scene, "show_entities")
        
        layout.separator()
        
        layout.operator("object.export_entities")
        
        if classname is None:
            layout.label(text="No entity selected")
            return
        
        layout.label(text=f"Classname: {classname}")
        
        sector_name = obj.entity_props.sector.name if obj.entity_props.sector else "None"
        layout.label(text=f"Sector: {sector_name}")
        layout.prop(obj.entity_props, "sector", text="")
        layout.operator("object.assign_selected_sector", text="Assign Selected")
        
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
        
        if classname.startswith("light") and LIGHT_STYLES:
            style_box = layout.box()
            # style_box.label(text="Style")
            style_box.prop(obj, "light_style")
        
        layout.use_property_split = False
        
        spawnflags = get_entity_spawnflags(classname)
        
        if spawnflags:
            spawnflags_box = layout.box()
            spawnflags_box.label(text="Spawnflags")
            
            for flag, label in spawnflags.items():
                prop_name = f"spawnflag_{flag}"
                
                if hasattr(obj, prop_name):
                    spawnflags_box.prop(obj, prop_name, text=label)


classes = (
    entity_properties,
    OBJECT_OT_assign_entity,
    OBJECT_OT_assign_selected_sector,
    OBJECT_OT_export_entities,
    OBJECT_OT_toggle_spawnflag,
    VIEW3D_PT_entity_panel,
)


draw_handler = None


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    for bit in FLAG_BITS:
        setattr(
            bpy.types.Object,
            f"spawnflag_{bit}",
            bpy.props.BoolProperty(
                name=f"Flag {bit}",
                get=make_flag_getter(bit),
                set=make_flag_setter(bit),
            ),
        )
    
    bpy.types.Object.light_style = bpy.props.EnumProperty(
        name="Style",
        items=get_light_style_items,
        get=get_light_style,
        set=set_light_style,
    )
    
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
        default=True
    )
    
    bpy.types.Scene.show_collision_boxes = bpy.props.BoolProperty(
        name="Show Collision Boxes",
        default=True
    )
    
    bpy.types.Scene.show_entities = bpy.props.BoolProperty(
        name="Show Entities",
        default=True,
        update=update_show_entities
    )
    
    global draw_handler
    
    draw_handler = bpy.types.SpaceView3D.draw_handler_add(
        draw_entity_overlay,
        (),
        'WINDOW',
        'POST_VIEW'
    )
    
    bpy.app.handlers.depsgraph_update_post.append(
        update_active_entity
    )


def unregister():
    for bit in FLAG_BITS:
        prop_name = f"spawnflag_{bit}"
        if hasattr(bpy.types.Object, prop_name):
            delattr(bpy.types.Object, prop_name)
    
    if hasattr(bpy.types.Object, "light_style"):
        delattr(bpy.types.Object, "light_style")
    
    del bpy.types.Object.entity_props
    del bpy.types.Scene.entity_definition_file
    del bpy.types.Scene.entity_category
    del bpy.types.Scene.entity_type
    del bpy.types.Scene.show_links
    del bpy.types.Scene.show_names
    del bpy.types.Scene.show_collision_boxes
    del bpy.types.Scene.show_entities
    
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