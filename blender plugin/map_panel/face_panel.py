import bpy
import bmesh

from collections import defaultdict
from mathutils import Vector
from mathutils.geometry import intersect_point_line

GRID_SIZE = 8.0
T_JUNCTION_EPSILON = 0.001

NORMAL_VERTEX = 0
VERTEX_INTERIOR = 1
VERTEX_T_EDGE = 2
VERTEX_BOUNDARY_EDGE = 4  # single vertices along a line
VERTEX_GEOMETRY_CORNER = 8
VERTEX_TEXTURE_CORNER = 16

NORMAL_EDGE = 0
EDGE_INTERIOR = 1
EDGE_T_GEOMETRY = 2
EDGE_T_TEXTURE = 4
EDGE_CORNER = 8

EDGE_INTERIOR_CORNER = EDGE_INTERIOR | EDGE_CORNER
EDGE_ALL_INTERIOR = EDGE_INTERIOR | EDGE_T_GEOMETRY | EDGE_T_TEXTURE | EDGE_CORNER

DEBUG = False

def is_boundary_edge(edge):
    # If there's more than two faces attaches
    if len(edge.link_faces) != 2:
        return True
    
    face0, face1 = edge.link_faces
    
    # If the faces doesn't share the material
    if face0.material_index != face1.material_index:
        return True
    
    # If the faces are not coplanar
    if face0.normal.dot(face1.normal) < 0.9999:
        return True
    
    return False


def classify_vertex(vert):
    boundary_edges = []
    
    for edge in vert.link_edges:
        if is_boundary_edge(edge):
            boundary_edges.append(edge)
    
    # Completely inside the coplanar region.
    if len(boundary_edges) == 0:
        return VERTEX_INTERIOR
    
    # A corner always has 3+ boundary edges.
    elif len(boundary_edges) >= 3:
        return VERTEX_GEOMETRY_CORNER
    
    # A middle vertex has exactly 2 collinear boundary edges.
    elif len(boundary_edges) == 2:
        edge0 = boundary_edges[0]
        edge1 = boundary_edges[1]
        
        dir0 = (edge0.other_vert(vert).co - vert.co).normalized()
        dir1 = (edge1.other_vert(vert).co - vert.co).normalized()
        
        if abs(dir0.dot(dir1)) > 0.9999:
            
            if len(vert.link_edges) == 2:
                return VERTEX_BOUNDARY_EDGE
            
            return VERTEX_T_EDGE
        
        return VERTEX_TEXTURE_CORNER
    
    # One boundary edge.
    if len(boundary_edges) == 1:
        
        # Straight boundary vertex.
        if len(vert.link_edges) == 2:
            return VERTEX_BOUNDARY_EDGE
        
        # T-junction.
        return VERTEX_T_EDGE
    
    return NORMAL_VERTEX


def classify_edge(edge):
    v0 = classify_vertex(edge.verts[0])
    v1 = classify_vertex(edge.verts[1])
    
    if v0 == VERTEX_T_EDGE or v1 == VERTEX_T_EDGE:
        if v0 == VERTEX_TEXTURE_CORNER or v1 == VERTEX_TEXTURE_CORNER:
            return EDGE_T_TEXTURE
        
        elif (
            (v0 == VERTEX_T_EDGE and v1 == VERTEX_T_EDGE) or
            v0 == VERTEX_GEOMETRY_CORNER or
            v1 == VERTEX_GEOMETRY_CORNER or
            v0 == VERTEX_INTERIOR or
            v1 == VERTEX_INTERIOR
        ):
            return EDGE_T_GEOMETRY
    
    elif (
        (v0 == VERTEX_INTERIOR and v1 == VERTEX_INTERIOR) or
        (v0 == VERTEX_INTERIOR and v1 == VERTEX_GEOMETRY_CORNER) or
        (v1 == VERTEX_INTERIOR and v0 == VERTEX_GEOMETRY_CORNER)
    ):
        return EDGE_INTERIOR
    
    elif (
        v0 == VERTEX_GEOMETRY_CORNER or
        v1 == VERTEX_GEOMETRY_CORNER or
        v0 == VERTEX_TEXTURE_CORNER or
        v1 == VERTEX_TEXTURE_CORNER
    ):
        return EDGE_CORNER
    
    return NORMAL_EDGE


def is_grid_vertex(vert, obj):
    matrix = obj.matrix_world
    grid_size = obj.map_props.grid_size
    p = matrix @ vert.co
    
    # A vertex is a grid vertex if it lies on any grid plane.
    for axis in range(3):
        grid_coordinate = round(p[axis] / grid_size) * grid_size
        
        if abs(p[axis] - grid_coordinate) <= 0.0001:
            return True
    
    return False


def is_grid_edge(edge, obj):
    matrix = obj.matrix_world
    grid_size = obj.map_props.grid_size
    
    p0 = matrix @ edge.verts[0].co
    p1 = matrix @ edge.verts[1].co
    
    for axis in range(3):
        # continue if the edge is not parallel to this grid plane
        if abs(p0[axis] - p1[axis]) > 0.0001:
            continue
        
        # nearest grid edge for p0
        grid_coordinate = round(p0[axis] / grid_size) * grid_size
        
        # continue if p0 is not on the plane
        if abs(p0[axis] - grid_coordinate) > 0.0001:
            continue
        
        face = edge.link_faces[0]
        
        # if one of the edge faces is not coplanar with the plane then it's a grid edge
        if abs(face.normal[axis]) < 0.9999:
            return True
    
    return False


def edge_has_uv_seam(edge, uv_layer):
    if len(edge.link_faces) != 2:
        return True
    
    face_loops = []
    for face in edge.link_faces:
        for loop in face.loops:
            if loop.edge == edge:
                face_loops.append(loop)
                break
    
    if len(face_loops) != 2:
        return True
    
    loop0, loop1 = face_loops
    
    for vert in edge.verts:
        uv0 = None
        uv1 = None
        
        for loop in (loop0, loop0.link_loop_next):
            if loop.vert == vert:
                uv0 = loop[uv_layer].uv
        
        for loop in (loop1, loop1.link_loop_next):
            if loop.vert == vert:
                uv1 = loop[uv_layer].uv
        
        if uv0 is None or uv1 is None:
            return True
        
        if (uv0 - uv1).length > 0.0001:
            return True
    
    return False


def vertex_has_uv_seam(vert, uv_layer):
    if uv_layer is None:
        return False
    
    loops = []
    
    for face in vert.link_faces:
        for loop in face.loops:
            if loop.vert == vert:
                loops.append(loop)
    
    if len(loops) < 2:
        return False
    
    uv = loops[0][uv_layer].uv
    
    for loop in loops[1:]:
        if (loop[uv_layer].uv - uv).length > 1e-6:
            return True
    
    return False


def get_grid_cell(position):
    return (
        int(position.x // GRID_SIZE),
        int(position.y // GRID_SIZE),
        int(position.z // GRID_SIZE),
    )


def build_edge_grid(bm):
    edge_grid = defaultdict(list)
    
    for edge in bm.edges:
        
        min_corner = Vector((
            min(edge.verts[0].co.x, edge.verts[1].co.x),
            min(edge.verts[0].co.y, edge.verts[1].co.y),
            min(edge.verts[0].co.z, edge.verts[1].co.z),
        ))
        
        max_corner = Vector((
            max(edge.verts[0].co.x, edge.verts[1].co.x),
            max(edge.verts[0].co.y, edge.verts[1].co.y),
            max(edge.verts[0].co.z, edge.verts[1].co.z),
        ))
        
        min_cell = get_grid_cell(min_corner)
        max_cell = get_grid_cell(max_corner)
        
        for x in range(min_cell[0], max_cell[0] + 1):
            for y in range(min_cell[1], max_cell[1] + 1):
                for z in range(min_cell[2], max_cell[2] + 1):
                    edge_grid[(x, y, z)].append(edge)
    
    return edge_grid


def get_vertices_by_type(bm, obj, vertex_type, avoid_grid=False):
    uv_layer = bm.loops.layers.uv.active
    matching_verts = []
    
    for vert in bm.verts:
        vert_class = classify_vertex(vert)
        
        if (vert_class & vertex_type) == 0:
            continue
        
        # Skip interior vertices with UV seams.
        if vertex_type == VERTEX_INTERIOR and vertex_has_uv_seam(vert, uv_layer):
            continue
        
        # Skip vertices that land on the grid
        if avoid_grid and is_grid_vertex(vert, obj):
            continue
        
        matching_verts.append(vert)
    
    return matching_verts


def get_edges_by_type(bm, obj, edge_type, avoid_grid=False):
    uv_layer = bm.loops.layers.uv.active
    matching_edges = []
    
    for edge in bm.edges:
        if is_boundary_edge(edge):
            continue
        
        # Skip UV seams
        if edge_has_uv_seam(edge, uv_layer):
            continue
        
        edge_class = classify_edge(edge)
        
        if (edge_class & edge_type) == 0:
            continue
        
        if avoid_grid and is_grid_edge(edge, obj):
            continue
        
        matching_edges.append(edge)
    
    return matching_edges

# UI selection

def select_vertices_by_type(bm, obj, vertex_type, avoid_grid=False, extend=False):
    if not extend:
        for vert in bm.verts:
            vert.select = False
            
    verts = get_vertices_by_type(bm, obj, vertex_type, avoid_grid=avoid_grid)
    for vert in verts:
        vert.select = True
        
    return len(verts)


def select_edges_by_type(bm, obj, edge_type, avoid_grid=False, extend=False):
    if not extend:
        for edge in bm.edges:
            edge.select = False
            
    edges = get_edges_by_type(bm, obj, edge_type, avoid_grid=avoid_grid)
    for edge in edges:
        edge.select = True
        
    return len(edges)


def is_convex_quad(verts, normal):
    for i in range(4):
        a = verts[i]
        b = verts[(i + 1) % 4]
        c = verts[(i + 2) % 4]
        
        if (b.co - a.co).cross(c.co - b.co).dot(normal) < 0:
            return False
    
    return True


def merge_triangles_to_quads(bm, obj):
    merged = 0
    
    select_edges_by_type(bm, obj, EDGE_ALL_INTERIOR, avoid_grid=True)
    
    candidate_edges = []
    
    # Collect selected edges and ensure both link faces are triangles
    candidate_edges = []
    for edge in bm.edges:
        if not edge.select:
            continue
            
        face0, face1 = edge.link_faces
        if len(face0.verts) == 3 and len(face1.verts) == 3:
            candidate_edges.append(edge)
    
    # Prefer shortest shared edges
    candidate_edges.sort(key=lambda e: e.calc_length())
    
    used_faces = set()
    
    for edge in candidate_edges:
        if not edge.is_valid or len(edge.link_faces) != 2:
            continue
        
        face0, face1 = edge.link_faces
        
        if face0 in used_faces or face1 in used_faces:
            continue
        
        if not face0.is_valid or not face1.is_valid:
            continue
        
        # Find opposite vertices
        edge_verts = set(edge.verts)
        opposite0 = next(v for v in face0.verts if v not in edge_verts)
        opposite1 = next(v for v in face1.verts if v not in edge_verts)
        
        # Derive true perimeter order using face0's loop orientation
        loop0 = next(l for l in face0.loops if l.vert == opposite0)
        v_next = loop0.link_loop_next.vert
        v_prev = loop0.link_loop_prev.vert
        
        # Continuous CCW perimeter: opposite0 -> v_next -> opposite1 -> v_prev
        quad_verts = (opposite0, v_next, opposite1, v_prev)
        
        if len(set(quad_verts)) != 4:
            continue
        
        # Test convexity on properly ordered vertices
        if not is_convex_quad(quad_verts, face0.normal):
            continue
        
        # Merge faces using bmesh utility
        new_face = bmesh.utils.face_join((face0, face1))
        
        if new_face:
            used_faces.add(new_face)
            merged += 1
    
    return merged


class FACEGRAB_OT_select_vertices(bpy.types.Operator):
    bl_idname = "facegrab.select_vertices"
    bl_label = "Select Vertices"
    
    mode: bpy.props.IntProperty(name="Mode", default=VERTEX_INTERIOR)
    avoid_grid: bpy.props.BoolProperty(name="Avoid Grid", default=False)
    extend: bpy.props.BoolProperty(name="Extend Selection", default=False)
    
    def execute(self, context):
        count = 0
        active = context.view_layer.objects.active
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_mode(type="VERT")
            
            bm = bmesh.from_edit_mesh(obj.data)
            count += select_vertices_by_type(
                bm, obj, self.mode, 
                avoid_grid=self.avoid_grid, 
                extend=self.extend
            )
            bmesh.update_edit_mesh(obj.data)
        
        context.view_layer.objects.active = active
        self.report({'INFO'}, f"Selected {count} vertex/vertices")
        return {'FINISHED'}


class FACEGRAB_OT_select_edges(bpy.types.Operator):
    bl_idname = "facegrab.select_edges"
    bl_label = "Select Edges"
    
    mode: bpy.props.IntProperty(name="Mode", default=NORMAL_EDGE)
    avoid_grid: bpy.props.BoolProperty(name="Avoid Grid", default=False)
    extend: bpy.props.BoolProperty(name="Extend Selection", default=False)
    
    def execute(self, context):
        count = 0
        active = context.view_layer.objects.active
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_mode(type="EDGE")
            
            bm = bmesh.from_edit_mesh(obj.data)
            count += select_edges_by_type(
                bm, obj, self.mode, 
                avoid_grid=self.avoid_grid, 
                extend=self.extend
            )
            bmesh.update_edit_mesh(obj.data)
        
        context.view_layer.objects.active = active
        self.report({'INFO'}, f"Selected {count} edge(s)")
        return {'FINISHED'}


class FACEGRAB_OT_fix_water_normals(bpy.types.Operator):
    bl_idname = "facegrab.fix_water_normals"
    bl_label = "Fix Water Normals"
    
    def execute(self, context):
        count = 0
        active = context.view_layer.objects.active
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            
            bm = bmesh.from_edit_mesh(obj.data)
            
            for face in bm.faces:
                mat = obj.data.materials[face.material_index]
                
                if not mat.name.startswith("*"):
                    continue
                
                if face.normal.z < 0.0:
                    face.normal_flip()
                    count += 1
            
            bmesh.update_edit_mesh(obj.data)
            
            bpy.ops.object.mode_set(mode='OBJECT')
        
        context.view_layer.objects.active = active
        
        self.report({'INFO'}, f"Flipped {count} normal(s)")
        return {'FINISHED'}


class FACEGRAB_OT_clean_internal_edges(bpy.types.Operator):
    bl_idname = "facegrab.clean_internal_edges"
    bl_label = "Clean Internal Edges"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active = context.view_layer.objects.active
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            
            bm = bmesh.from_edit_mesh(obj.data)
            
            # Dissolve interior vertices
            verts = get_vertices_by_type(bm, obj, VERTEX_INTERIOR)
            if verts:
                bmesh.ops.dissolve_verts(bm, verts=verts, use_face_split=True)
            
            # Dissolve boundary vertices
            verts = get_vertices_by_type(bm, obj, VERTEX_BOUNDARY_EDGE)
            if verts:
                bmesh.ops.dissolve_verts(bm, verts=verts, use_face_split=True)
            
            # Dissolve interior edges
            edges = get_edges_by_type(bm, obj, EDGE_INTERIOR)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=True, use_face_split=True)
            
            # Dissolve geometry T edges twice
            for i in range(2):
                edges = get_edges_by_type(bm, obj, EDGE_T_GEOMETRY)
                if edges:
                    bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=True, use_face_split=True)
            
            # Triangulate all faces in memory
            bmesh.ops.triangulate(
                bm,
                faces=bm.faces[:],
                quad_method='BEAUTY',
                ngon_method='BEAUTY'
            )
            
            # Dissolve texture T edges
            edges = get_edges_by_type(bm, obj, EDGE_T_TEXTURE)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=False, use_face_split=True)
            
            # Dissolve geometry T edges
            edges = get_edges_by_type(bm, obj, EDGE_T_GEOMETRY)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=True, use_face_split=True)
            
            # Dissolve corner edges
            edges = get_edges_by_type(bm, obj, EDGE_CORNER)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=False, use_face_split=False)
            
            # Sync memory buffer back to viewport
            bmesh.update_edit_mesh(obj.data)
        
        context.view_layer.objects.active = active
        self.report({'INFO'}, "Cleaned the internal edges")
        return {'FINISHED'}


class FACEGRAB_OT_clean_grid_edges(bpy.types.Operator):
    bl_idname = "facegrab.clean_grid_edges"
    bl_label = "Clean Grid Edges"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active = context.view_layer.objects.active
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            
            bm = bmesh.from_edit_mesh(obj.data)
            
            # Dissolve the interior edges twice
            edges = get_edges_by_type(bm, obj, EDGE_INTERIOR, avoid_grid=True)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=True, use_face_split=False)
            
            edges = get_edges_by_type(bm, obj, EDGE_INTERIOR, avoid_grid=True)
            if edges:
                bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=True, use_face_split=True)
            
            for edge in bm.edges:
                edge.select = False
            
            bmesh.update_edit_mesh(obj.data)
        
        context.view_layer.objects.active = active
        self.report({'INFO'}, "Cleaned non-grid internal edges")
        return {'FINISHED'}


class FACEGRAB_OT_subdivide_quads(bpy.types.Operator):
    bl_idname = "facegrab.subdivide_quads"
    bl_label = "Subdivide into Quads"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active = context.view_layer.objects.active
        total = 0
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            
            # Triangulate the mesh.
            bm = bmesh.from_edit_mesh(obj.data)
            
            bmesh.ops.triangulate(
                bm,
                faces=bm.faces[:],
                quad_method='BEAUTY',
                ngon_method='BEAUTY'
            )
            
            # Merge the triangles into quads
            total += merge_triangles_to_quads(bm, obj)
            
            bmesh.update_edit_mesh(obj.data)
        
        context.view_layer.objects.active = active
        
        self.report(
            {'INFO'},
            f"Merged {total} triangle pair(s) into quads"
        )
        
        return {'FINISHED'}


class FACEGRAB_OT_clean_t_junctions(bpy.types.Operator):
    bl_idname = "facegrab.clean_t_junctions"
    bl_label = "Clean T-Junctions"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        obj = context.active_object
        
        if obj is None or obj.type != 'MESH':
            return {'CANCELLED'}
        
        bpy.ops.object.mode_set(mode='EDIT')
        bm = bmesh.from_edit_mesh(obj.data)
        
        # Spatial partition grid for edge lookup
        edge_grid = build_edge_grid(bm)
        edge_splits = defaultdict(list)
        
        # Find all T-junctions
        for vertex in bm.verts:
            checked_edges = set()
            
            # Search neighboring grid cells
            min_cell = get_grid_cell(vertex.co - Vector((0.0001, 0.0001, 0.0001)))
            max_cell = get_grid_cell(vertex.co + Vector((0.0001, 0.0001, 0.0001)))
            
            for x in range(min_cell[0], max_cell[0] + 1):
                for y in range(min_cell[1], max_cell[1] + 1):
                    for z in range(min_cell[2], max_cell[2] + 1):
                        
                        for edge in edge_grid.get((x, y, z), ()):
                            if edge in checked_edges or vertex in edge.verts:
                                continue
                            
                            checked_edges.add(edge)
                            
                            # Projection calculation
                            closest_pt, projection = intersect_point_line(
                                vertex.co,
                                edge.verts[0].co,
                                edge.verts[1].co
                            )
                            
                            # Must project inside the edge segment (0.0 < t < 1.0)
                            if 0.0 < projection < 1.0:
                                if (vertex.co - closest_pt).length <= T_JUNCTION_EPSILON:
                                    edge_splits[edge].append((projection, vertex))
        
        fixed_t_junctions = 0
        
        # Split edges at T-junction points
        for edge, splits in edge_splits.items():
            splits.sort(key=lambda split: split[0])
            
            current_edge = edge
            start_vertex = edge.verts[0]
            previous_projection = 0.0
            
            for projection, vertex in splits:
                local_projection = (projection - previous_projection) / (1.0 - previous_projection)
                
                current_edge, _ = bmesh.utils.edge_split(
                    current_edge,
                    start_vertex,
                    local_projection,
                )
                
                start_vertex = current_edge.verts[0]
                previous_projection = projection
                fixed_t_junctions += 1
        
        # Weld newly split vertices to stitch the T-junctions
        if fixed_t_junctions > 0:
            bmesh.ops.weld_verts(bm, targetmode='ALL', dist=T_JUNCTION_EPSILON)
        
        bmesh.update_edit_mesh(obj.data)
        
        self.report({'INFO'}, f"Fixed {fixed_t_junctions} T-junction(s)")
        return {'FINISHED'}


class VIEW3D_PT_face_panel(bpy.types.Panel):
    bl_label = "Faces"
    bl_idname = "VIEW3D_PT_face_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Faces"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object
        scene = context.scene
        
        # layout.use_property_split = True
        # layout.use_property_decorate = False
        
        if obj is None or obj.type != 'MESH':
            layout.label(text="No mesh selected")
            return
        
        layout.operator("facegrab.fix_water_normals", text="Fix Water Normals")
        layout.operator("facegrab.clean_internal_edges", text="Clean Internal Edges")
        layout.operator("facegrab.clean_grid_edges", text="Clean Grid Edges")
        layout.operator("facegrab.subdivide_quads", text="Subdivide into Quads")
        layout.operator("facegrab.clean_t_junctions", text="Clean T Junctions")
        
        if DEBUG:
            layout.separator()
            
            # Debug Checkboxes
            layout.prop(scene, "debug_avoid_grid", text="Avoid Grid")
            layout.prop(scene, "debug_extend_selection", text="Extend Selection")
            layout.separator()
            
            # Helper inline functions to pass checkbox states to operators
            def add_edge_op(text, mode):
                op = layout.operator("facegrab.select_edges", text=text)
                op.mode = mode
                op.avoid_grid = scene.debug_avoid_grid
                op.extend = scene.debug_extend_selection
            
            def add_vert_op(text, mode):
                op = layout.operator("facegrab.select_vertices", text=text)
                op.mode = mode
                op.avoid_grid = scene.debug_avoid_grid
                op.extend = scene.debug_extend_selection
            
            add_edge_op("Select Interior Edges", EDGE_INTERIOR)
            add_edge_op("Select Geometry T Edges", EDGE_T_GEOMETRY)
            add_edge_op("Select Texture T Edges", EDGE_T_TEXTURE)
            add_edge_op("Select Corner Edges", EDGE_CORNER)
            add_edge_op("Select Interior Corner Edges", EDGE_INTERIOR_CORNER)
            
            layout.separator()
            
            add_vert_op("Select Interior Vertices", VERTEX_INTERIOR)
            add_vert_op("Select T Vertices", VERTEX_T_EDGE)
            add_vert_op("Select Geometry Corner Vertices", VERTEX_GEOMETRY_CORNER)
            add_vert_op("Select Texture Corner Vertices", VERTEX_TEXTURE_CORNER)
            add_vert_op("Select Boundary Edge Vertices", VERTEX_BOUNDARY_EDGE)


classes = (
    FACEGRAB_OT_select_vertices,
    FACEGRAB_OT_select_edges,
    FACEGRAB_OT_fix_water_normals,
    FACEGRAB_OT_clean_internal_edges,
    FACEGRAB_OT_clean_grid_edges,
    FACEGRAB_OT_subdivide_quads,
    FACEGRAB_OT_clean_t_junctions,
    VIEW3D_PT_face_panel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Scene.debug_avoid_grid = bpy.props.BoolProperty(
        name="Avoid Grid",
        default=False
    )
    
    bpy.types.Scene.debug_extend_selection = bpy.props.BoolProperty(
        name="Extend Selection",
        default=False
    )


def unregister():
    del bpy.types.Scene.debug_avoid_grid
    del bpy.types.Scene.debug_extend_selection
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)