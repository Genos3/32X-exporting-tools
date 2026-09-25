import bpy
import bmesh

from .raytracer_gpu import bake_lighting

import gpu
from gpu_extras.batch import batch_for_shader
from mathutils import Vector

items = [
    ('NONE', "None", ""),
    ('SECTOR', "Sector", ""),
    ('PORTAL', "Portal", ""),
    ('BARRIER', "Barrier", ""),
    ('ENTITY', "Entity", ""),
]

PORTAL_COLOR = (0.0, 0.7, 1.0, 0.5)
BARRIER_COLOR = (1.0, 0.7, 0.0, 0.5)

CUSTOM_BAKING = False
ENABLE_DEPTH_BUFFER = True


def get_portal_material():
    mat = bpy.data.materials.get("Portal")
    
    if mat is None:
        mat = bpy.data.materials.new("Portal")
        mat.use_nodes = True
        mat.blend_method = 'BLEND'
        mat.shadow_method = 'NONE'
        mat.diffuse_color = (0.0, 0.7, 1.0, 1.0)
        mat.use_backface_culling = False
        
        bsdf = mat.node_tree.nodes["Principled BSDF"]
        bsdf.inputs["Base Color"].default_value = (0.0, 0.7, 1.0, 1.0)
        bsdf.inputs["Alpha"].default_value = 0.25
    
    return mat


def get_barrier_material():
    mat = bpy.data.materials.get("Barrier")
    
    if mat is None:
        mat = bpy.data.materials.new("Barrier")
        mat.use_nodes = True
        mat.blend_method = 'BLEND'
        mat.shadow_method = 'NONE'
        mat.diffuse_color = (1.0, 0.7, 0.0, 1.0)
        mat.use_backface_culling = False
        
        bsdf = mat.node_tree.nodes["Principled BSDF"]
        bsdf.inputs["Base Color"].default_value = (1.0, 0.7, 0.0, 1.0)
        bsdf.inputs["Alpha"].default_value = 0.25
    
    return mat


def draw_map_boundaries():
    portal_vertices = []
    barrier_vertices = []
    
    for obj in bpy.context.scene.objects:
        if obj.hide_get() or not obj.visible_get() or obj.type != 'MESH':
            continue
        
        map_type = obj.map_props.map_type
        
        if map_type not in {'PORTAL', 'BARRIER'}:
            continue
        
        vertices = (
            portal_vertices
            if map_type == 'PORTAL'
            else barrier_vertices
        )
        
        for edge in obj.data.edges:
            v0 = obj.matrix_world @ obj.data.vertices[edge.vertices[0]].co
            v1 = obj.matrix_world @ obj.data.vertices[edge.vertices[1]].co
            
            vertices.append(v0)
            vertices.append(v1)
    
    shader = gpu.shader.from_builtin('UNIFORM_COLOR')
    shader.bind()
    
    gpu.state.depth_test_set(
        'LESS_EQUAL' if ENABLE_DEPTH_BUFFER else 'NONE'
    )
    gpu.state.blend_set('ALPHA')
    gpu.state.line_width_set(2.0)
    
    if portal_vertices:
        shader.uniform_float("color", PORTAL_COLOR)
        batch = batch_for_shader(shader, 'LINES', {"pos": portal_vertices})
        batch.draw(shader)
    
    if barrier_vertices:
        shader.uniform_float("color", BARRIER_COLOR)
        batch = batch_for_shader(shader, 'LINES', {"pos": barrier_vertices})
        batch.draw(shader)
    
    gpu.state.line_width_set(1.0)
    gpu.state.blend_set('NONE')


class map_properties(bpy.types.PropertyGroup):
    map_type: bpy.props.EnumProperty(
        name="Type",
        items=items,
        default='NONE'
    )
    
    has_lighting: bpy.props.BoolProperty(
        name="Lighting",
        default=True
    )
    
    has_grid: bpy.props.BoolProperty(
        name="Grid",
        default=False
    )
    
    bake_mode: bpy.props.EnumProperty(
        name="Bake Mode",
        items=[
            ('VERTEX', "Per Vertex", ""),
            ('FACE', "Per Face", ""),
        ],
        default='VERTEX'
    )
    
    grid_size: bpy.props.FloatProperty(
        name="Grid Size",
        description="Size of the grid cells to slice the mesh into",
        default=2.0,
        min=1.0
    )


class OBJECT_OT_assign_map_type(bpy.types.Operator):
    bl_idname = "object.assign_map_type"
    bl_label = "Assign to Selected"
    
    def execute(self, context):
        active = context.object
        map_type = active.map_props.map_type
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            obj.map_props.map_type = map_type
            
            if map_type == 'PORTAL':
                mat = get_portal_material()
                
                obj.data.materials.clear()
                obj.data.materials.append(mat)
                
                for poly in obj.data.polygons:
                    poly.material_index = 0
            
            if map_type == 'BARRIER':
                mat = get_barrier_material()
                
                obj.data.materials.clear()
                obj.data.materials.append(mat)
                
                for poly in obj.data.polygons:
                    poly.material_index = 0
        
        self.report({'INFO'}, f"Map type assigned")
        return {'FINISHED'}


class OBJECT_OT_map_bake_lighting(bpy.types.Operator):
    bl_idname = "object.map_bake_lighting"
    bl_label = "Bake Lighting"
    
    def execute(self, context):
        scene = context.scene        
        render_states = {}
        
        # Hide flickering lights and brush entities (doors, platforms, etc.)
        for obj in scene.objects:
            render_states[obj] = obj.hide_render
            
            if obj.type == 'LIGHT':
                style = int(obj.get("style", 0))
                classname = obj.get("classname", "")
                
                # Quake light styles > 0 are dynamic/flickering sequences.
                # Hide them from the static vertex bake.
                if style != 0 or classname == "light_fluorospark":
                    obj.hide_render = True
            
            elif obj.type == 'MESH':
                classname = obj.get("classname", "")
                
                if classname.startswith(("func_door", "func_plat", "func_train", "func_water", "func_illusionary")):
                    obj.hide_render = True
        
        try:
            selected_meshes = [
                obj for obj in context.selected_objects 
                if obj.type == 'MESH'
            ]
            
            # Prepare vertex color attributes on target mesh objects
            for obj in context.selected_objects:
                if obj.type != 'MESH':
                    continue
                
                if "Lighting" in obj.data.color_attributes:
                    obj.data.color_attributes.remove(
                        obj.data.color_attributes["Lighting"]
                    )
                
                col_attr = obj.data.color_attributes.new(
                    name="Lighting",
                    type='BYTE_COLOR',
                    # domain='CORNER'
                    domain='POINT'
                )
                
                obj.data.color_attributes.active_color = col_attr
            
            if not CUSTOM_BAKING:
                # Bake direct & indirect light to vertex colors
                bpy.ops.object.bake(
                    type='DIFFUSE',
                    pass_filter={'DIRECT', 'INDIRECT'},
                    target='VERTEX_COLORS'
                )
            else:
                # custom raytracing function
                bake_lighting(scene, selected_meshes)
            
            # Average the baked vertex colors per face
            if context.object.map_props.bake_mode == 'FACE':
                for obj in context.selected_objects:
                    if obj.type != 'MESH':
                        continue
                    
                    mesh = obj.data
                    color_attr = mesh.color_attributes.get("Lighting")
                    
                    if not color_attr:
                        continue
                        
                    for poly in mesh.polygons:
                        r = g = b = 0.0
                        
                        # Sum the colors of all vertices in this face
                        for loop_idx in poly.loop_indices:
                            color = color_attr.data[loop_idx].color
                            r += color[0]
                            g += color[1]
                            b += color[2]
                        
                        # Calculate the average
                        count = poly.loop_total
                        avg_color = (r / count, g / count, b / count, 1.0)
                        
                        # Apply the averaged color back to the face's vertices
                        for loop_idx in poly.loop_indices:
                            color_attr.data[loop_idx].color = avg_color
                        
                        """
                        for poly in mesh.polygons:
                        r = g = b = 0.0
                        
                        # Sum colors using vertex indices
                        for v_idx in poly.vertices:
                            color = color_attr.data[v_idx].color
                            r += color[0]
                            g += color[1]
                            b += color[2]
                        
                        count = len(poly.vertices)
                        avg_color = (r / count, g / count, b / count, 1.0)
                        
                        # Write back using vertex indices
                        for v_idx in poly.vertices:
                            color_attr.data[v_idx].color = avg_color
                        """
        
        finally:
            # Restore original object visibility
            for obj, original_hide in render_states.items():
                obj.hide_render = original_hide
        
        # Switch 3D Viewport shading to display the baked color attribute
        for area in context.screen.areas:
            if area.type == 'VIEW_3D':
                for space in area.spaces:
                    if space.type == 'VIEW_3D':
                        space.shading.type = 'SOLID'
                        space.shading.color_type = 'VERTEX'
        
        self.report({'INFO'}, f"Lighting baked")
        return {'FINISHED'}


class OBJECT_OT_map_export_lighting(bpy.types.Operator):
    bl_idname = "object.map_export_lighting"
    bl_label = "Export Lighting"
    
    lighting_mode: bpy.props.EnumProperty(
        name="Lighting",
        items=[
            ('FACE', "Per Face", ""),
            ('VERTEX', "Per Vertex", ""),
        ],
        default='VERTEX'
    )
    
    def invoke(self, context, event):
        return context.window_manager.invoke_props_dialog(self)
    
    def draw(self, context):
        self.layout.prop(self, "lighting_mode")
    
    def execute(self, context):
        print(self.lighting_mode)
        return {'FINISHED'}


class OBJECT_OT_map_slice_grid(bpy.types.Operator):
    bl_idname = "object.map_slice_grid"
    bl_label = "Slice with Grid"
    bl_description = "Slices the selected mesh objects into a grid based on the Grid Size"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        import math
        import bmesh
        from mathutils import Vector
        
        if not context.selected_objects:
            self.report({'WARNING'}, "No objects selected")
            return {'CANCELLED'}
        
        if context.object and context.object.mode == 'EDIT':
            bpy.ops.object.mode_set(mode='OBJECT')
        
        sliced_count = 0
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            # Skip objects that are marked as entities (like doors, platforms, triggers, etc.)
            if obj.map_props.map_type == 'ENTITY':
                continue
            
            grid_size = obj.map_props.grid_size
            if grid_size <= 0.001:
                continue
            
            # Load mesh data into bmesh
            bm = bmesh.new()
            bm.from_mesh(obj.data)
            
            # Transform the bmesh into global/world space before slicing
            bm.transform(obj.matrix_world)
            
            # Calculate the bounding box for slicing boundaries
            min_v = Vector((float('inf'), float('inf'), float('inf')))
            max_v = Vector((-float('inf'), -float('inf'), -float('inf')))
            
            for v in bm.verts:
                for i in range(3):
                    if v.co[i] < min_v[i]: min_v[i] = v.co[i]
                    if v.co[i] > max_v[i]: max_v[i] = v.co[i]
            
            # Helper function to slice sequentially along a specific axis (X=0, Y=1, Z=2)
            def slice_axis(axis_idx, start, end, size):
                current = math.floor(start / size) * size + size
                
                while current < end - 0.001:
                    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
                    
                    plane_co = [0, 0, 0]
                    plane_co[axis_idx] = current
                    
                    plane_no = [0, 0, 0]
                    plane_no[axis_idx] = 1
                    
                    bmesh.ops.bisect_plane(
                        bm, 
                        geom=geom, 
                        dist=0.001, 
                        plane_co=plane_co, 
                        plane_no=plane_no, 
                        clear_inner=False, 
                        clear_outer=False
                    )
                    current += size
            
            # Execute slices across X, Y, and Z axes
            slice_axis(0, min_v.x, max_v.x, grid_size)
            slice_axis(1, min_v.y, max_v.y, grid_size)
            slice_axis(2, min_v.z, max_v.z, grid_size)
            
            # Transform the bmesh back into local space before saving
            bm.transform(obj.matrix_world.inverted())
            
            # Write data back and update object
            bm.to_mesh(obj.data)
            bm.free()
            obj.data.update()
            
            sliced_count += 1
        
        if context.object and context.object.type == 'MESH':
            bpy.ops.object.mode_set(mode='EDIT')
        
        self.report({'INFO'}, f"Sliced {sliced_count} mesh(es)")
        
        return {'FINISHED'}


class VIEW3D_PT_map_panel(bpy.types.Panel):
    bl_label = "Map"
    bl_idname = "VIEW3D_PT_map_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Map"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object
        
        layout.use_property_split = True
        layout.use_property_decorate = False
        
        if obj is None:
            layout.label(text="No object selected")
            return
        
        props = obj.map_props
        
        layout.prop(props, "map_type", text="Type")
        layout.operator("object.assign_map_type")
        layout.prop(props, "has_lighting", text="Has Lighting")
        layout.prop(props, "has_grid", text="Has Grid")
        
        layout.separator()
        
        layout.use_property_split = False
        layout.label(text=f"Bake Mode")
        layout.prop(props, "bake_mode", expand=True)
        
        layout.operator("object.map_bake_lighting")
        # layout.separator()
        # layout.operator("object.map_export_lighting")
        
        layout.separator()
        
        layout.prop(props, "grid_size")
        layout.operator("object.map_slice_grid")


classes = (
    map_properties,
    OBJECT_OT_assign_map_type,
    OBJECT_OT_map_bake_lighting,
    OBJECT_OT_map_export_lighting,
    OBJECT_OT_map_slice_grid,
    VIEW3D_PT_map_panel,
)


draw_handler = None


def register():
    global draw_handler
    
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Object.map_props = bpy.props.PointerProperty(
        type=map_properties
    )
    
    draw_handler = bpy.types.SpaceView3D.draw_handler_add(
        draw_map_boundaries,
        (),
        'WINDOW',
        'POST_VIEW'
    )


def unregister():
    global draw_handler
    
    if draw_handler is not None:
        bpy.types.SpaceView3D.draw_handler_remove(
            draw_handler,
            'WINDOW'
        )
        draw_handler = None
    
    del bpy.types.Object.map_props
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)