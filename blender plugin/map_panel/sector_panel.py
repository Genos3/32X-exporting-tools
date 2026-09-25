import bpy
import bmesh
from mathutils import Vector
from mathutils.geometry import tessellate_polygon

from .map_panel import get_portal_material, get_barrier_material
from .entity_panel import get_liquid_types

DEBUG = False
DEBUG_CUT = False


def get_world_aabb(obj):
    corners = [
        obj.matrix_world @ Vector(corner)
        for corner in obj.bound_box
    ]
    
    min_corner = Vector((
        min(v.x for v in corners),
        min(v.y for v in corners),
        min(v.z for v in corners),
    ))
    
    max_corner = Vector((
        max(v.x for v in corners),
        max(v.y for v in corners),
        max(v.z for v in corners),
    ))
    
    return min_corner, max_corner


def point_in_polygon(point, poly_verts, poly_normal, epsilon=1e-4):
    # Works for convex polygons (quads are fine).
    for i in range(len(poly_verts)):
        a = poly_verts[i]
        b = poly_verts[(i + 1) % len(poly_verts)]
        
        if (b - a).cross(point - a).dot(poly_normal) < -epsilon:
            return False
    
    return True


def edge_plane_intersect(v1, v2, plane_co, plane_no, epsilon=1e-4):
    d1 = (v1 - plane_co).dot(plane_no)
    d2 = (v2 - plane_co).dot(plane_no)
    
    # Both endpoints lie on the plane — not a real crossing, skip.
    if abs(d1) < epsilon and abs(d2) < epsilon:
        return None
    
    # One endpoint lies on the plane — treat that endpoint as the intersection.
    if abs(d1) < epsilon:
        return v1
    if abs(d2) < epsilon:
        return v2
    
    if d1 * d2 >= 0:  # same side, no crossing
        return None
    
    t = d1 / (d1 - d2)
    return v1.lerp(v2, t)


def point_on_plane(point, plane_point, plane_normal):
    return abs((point - plane_point).dot(plane_normal)) < 0.0001


def edge_lies_on_boundary(edge, plane_point, plane_normal, boundary_verts):
    v1 = edge.verts[0].co
    v2 = edge.verts[1].co
    
    if not point_on_plane(v1, plane_point, plane_normal):
        return False
    if not point_on_plane(v2, plane_point, plane_normal):
        return False
    
    if not point_in_polygon(v1, boundary_verts, plane_normal):
        return False
    if not point_in_polygon(v2, boundary_verts, plane_normal):
        return False
    
    return True


def is_liquid_face(face, main_obj):
    material = main_obj.data.materials[face.material_index]
    return material and material.name.startswith("*")


def collect_boundary_data(main_obj, portal_coll, barrier_coll, selected_only):
    boundaries_data = []
    
    for coll in (portal_coll, barrier_coll):
        if not coll:
            continue
        
        for boundary in coll.objects:
            if boundary.type != 'MESH' or boundary == main_obj:
                continue
            if selected_only and not boundary.select_get():
                continue
            if not boundary.data.polygons:
                continue
            
            poly = boundary.data.polygons[0]
            
            matrix_boundary_to_main = (
                main_obj.matrix_world.inverted() @ boundary.matrix_world
            )
            normal_matrix = matrix_boundary_to_main.to_3x3().inverted().transposed()
            
            local_point = matrix_boundary_to_main @ poly.center
            local_normal = (normal_matrix @ poly.normal).normalized()
            
            local_boundary_verts = [
                matrix_boundary_to_main @ boundary.data.vertices[v_idx].co
                for v_idx in poly.vertices
            ]
            
            boundaries_data.append((
                local_point,
                local_normal,
                local_boundary_verts
            ))
    
    return boundaries_data


def test_select_affected_faces(main_obj, portal_coll, barrier_coll, selected_only, report):
    # Switch to Edit Mode to work with face selection
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Get active BMesh from the mesh in Edit Mode
    bm = bmesh.from_edit_mesh(main_obj.data)
    
    # Deselect everything first
    bpy.ops.mesh.select_all(action='DESELECT')
    
    boundaries_data = collect_boundary_data(
        main_obj,
        portal_coll,
        barrier_coll,
        selected_only
    )
    
    if not boundaries_data:
        report({'ERROR'}, "No portals or barriers found to process")
        return
    
    total_faces_selected = 0
    
    for local_point, local_normal, local_boundary_verts in boundaries_data:
        bm.faces.ensure_lookup_table()
        bm.edges.ensure_lookup_table()
        
        for face in bm.faces:
            for edge in face.edges:
                v1 = edge.verts[0].co
                v2 = edge.verts[1].co
                
                isect = edge_plane_intersect(v1, v2, local_point, local_normal)
                if isect is None:
                    continue
                
                if point_in_polygon(isect, local_boundary_verts, local_normal):
                    # Select the face in the viewport
                    face.select = True
                    total_faces_selected += 1
                    break
    
    # Update viewport selection
    bmesh.update_edit_mesh(main_obj.data)
    
    report({'INFO'}, f"Selected {total_faces_selected} face(s) across {len(boundaries_data)} portal(s)")


def split_edge_at_point(edge, point):
    v0 = edge.verts[0]
    v1 = edge.verts[1]
    
    direction = v1.co - v0.co
    length_squared = direction.length_squared
    
    if length_squared == 0.0:
        return v0
    
    factor = (point - v0.co).dot(direction) / length_squared
    
    if factor <= 0.0:
        return v0
    
    if factor >= 1.0:
        return v1
    
    new_edge, new_vertex = bmesh.utils.edge_split(
        edge,
        v0,
        factor
    )
    
    return new_vertex


def is_face_coplanar(face, plane_co, plane_normal, epsilon=1e-4):
    if abs(abs(face.normal.dot(plane_normal)) - 1.0) >= epsilon:
        return False
    
    return abs(
        (face.verts[0].co - plane_co).dot(plane_normal)
    ) < epsilon


def vertices_connected_on_plane(
    face,
    vertex0,
    vertex1,
    plane_co,
    plane_normal,
    epsilon=1e-4
):
    current = vertex0
    visited = {vertex0}
    
    while current != vertex1:
        next_vertex = None
        
        for edge in current.link_edges:
            if edge not in face.edges:
                continue
            
            other_vertex = edge.other_vert(current)
            
            if other_vertex in visited:
                continue
            
            if abs(
                (other_vertex.co - plane_co).dot(plane_normal)
            ) > epsilon:
                continue
            
            next_vertex = other_vertex
            break
        
        if next_vertex is None:
            return False
        
        visited.add(next_vertex)
        current = next_vertex
    
    return True


def point_in_concave_polygon(point, poly_verts, poly_normal, epsilon=1e-4):
    # Project onto the two axes least aligned with the face normal.
    normal = Vector((
        abs(poly_normal.x),
        abs(poly_normal.y),
        abs(poly_normal.z)
    ))
    
    if normal.x >= normal.y and normal.x >= normal.z:
        axis_u = 1
        axis_v = 2
    elif normal.y >= normal.z:
        axis_u = 0
        axis_v = 2
    else:
        axis_u = 0
        axis_v = 1
    
    point_u = point[axis_u]
    point_v = point[axis_v]
    
    inside = False
    
    for index in range(len(poly_verts)):
        a = poly_verts[index]
        b = poly_verts[(index + 1) % len(poly_verts)]
        
        a_u = a[axis_u]
        a_v = a[axis_v]
        b_u = b[axis_u]
        b_v = b[axis_v]
        
        # Point lies on the polygon edge.
        edge = b - a
        to_point = point - a
        
        if edge.cross(to_point).length <= epsilon:
            if (
                min(a_u, b_u) - epsilon <= point_u <= max(a_u, b_u) + epsilon
                and
                min(a_v, b_v) - epsilon <= point_v <= max(a_v, b_v) + epsilon
            ):
                return True
        
        # Ray crossing test.
        if (a_v > point_v) != (b_v > point_v):
            intersection_u = (
                a_u
                + (point_v - a_v)
                * (b_u - a_u)
                / (b_v - a_v)
            )
            
            if point_u < intersection_u:
                inside = not inside
    
    return inside


def clip_mesh_with_boundary(
    bm,
    boundary_verts,
    boundary_normal,
    epsilon=1e-4
):
    boundary_co = boundary_verts[0]
    edge_intersections = []
    
    # Find mesh edges crossed by the boundary plane.
    for edge in list(bm.edges):
        intersection = edge_plane_intersect(
            edge.verts[0].co,
            edge.verts[1].co,
            boundary_co,
            boundary_normal,
            epsilon
        )
        
        if intersection is None:
            continue
        
        if not point_in_polygon(
            intersection,
            boundary_verts,
            boundary_normal,
            epsilon
        ):
            continue
        
        edge_intersections.append((edge, intersection))
    
    intersection_vertices = set()
    
    # Split the crossed edges at their exact intersection points.
    for edge, intersection in edge_intersections:
        vertex = split_edge_at_point(edge, intersection)
        intersection_vertices.add(vertex)
    
    bm.verts.ensure_lookup_table()
    bm.edges.ensure_lookup_table()
    bm.faces.ensure_lookup_table()
    
    if DEBUG_CUT:
        for vertex in intersection_vertices:
            vertex.select = True
        
        bpy.ops.object.mode_set(mode='MESH')
        bpy.ops.mesh.select_mode(type='VERTICES')
        
        return set(), 0
    
    boundary_edges = set()
    affected_faces = set()
    
    # Connect the boundary intersections across each affected face.
    for face in list(bm.faces):
        if is_face_coplanar(
            face,
            boundary_co,
            boundary_normal,
            epsilon
        ):
            continue
        
        face_vertices = [
            loop.vert.co.copy()
            for loop in face.loops
        ]
        
        intersection_vertices_on_face = []
        
        for loop in face.loops:
            vertex = loop.vert
            
            if vertex in intersection_vertices:
                intersection_vertices_on_face.append(vertex)
        
        if len(intersection_vertices_on_face) < 2:
            continue
        
        # A clean slice through a convex face produces exactly 2 perimeter intersection points.
        if len(intersection_vertices_on_face) == 2:
            vertex0 = intersection_vertices_on_face[0]
            vertex1 = intersection_vertices_on_face[1]
            
            edge = bm.edges.get((vertex0, vertex1))
            
            if edge is not None:
                boundary_edges.add(edge)
            else:
                result = bmesh.ops.connect_verts(
                    bm,
                    verts=[vertex0, vertex1]
                )
                
                boundary_edges.update(
                    result.get("edges", [])
                )
                affected_faces.add(face)
        
        # Fallback for complex n-gons or perfectly aligned edges with multiple collinear split points
        elif len(intersection_vertices_on_face) > 2:
            # Intersection points on a face always lie on a single straight line.
            # Find the vector direction of that line using the first two distinct points.
            line_dir = None
            v0_co = intersection_vertices_on_face[0].co
            
            for v in intersection_vertices_on_face[1:]:
                diff = v.co - v0_co
                if diff.length_squared > 1e-6:
                    line_dir = diff.normalized()
                    break
            
            if line_dir is None:
                continue
            
            # Sort the vertices spatially along the line so they pair correctly
            if line_dir:
                intersection_vertices_on_face.sort(key=lambda v: (v.co - v0_co).dot(line_dir))
            
            # Connect the sorted, sequential boundary vertex pairs
            for index in range(len(intersection_vertices_on_face) - 1):
                vertex0 = intersection_vertices_on_face[index]
                vertex1 = intersection_vertices_on_face[index + 1]
                
                midpoint = (vertex0.co + vertex1.co) * 0.5
                
                if not point_in_concave_polygon(
                    midpoint,
                    face_vertices,
                    face.normal,
                    epsilon
                ):
                    continue
                
                edge = bm.edges.get((vertex0, vertex1))
                
                if edge is not None:
                    boundary_edges.add(edge)
                else:
                    result = bmesh.ops.connect_verts(
                        bm,
                        verts=[vertex0, vertex1]
                    )
                    
                    boundary_edges.update(
                        result.get("edges", [])
                    )
                    
                    affected_faces.add(face)
    
    return boundary_edges, len(affected_faces)


def slice_with_boundaries(main_obj, portal_coll, barrier_coll, selected_only, report):
    bm = bmesh.new()
    bm.from_mesh(main_obj.data)
    
    boundaries_data = collect_boundary_data(
        main_obj,
        portal_coll,
        barrier_coll,
        selected_only
    )
    
    if not boundaries_data:
        report({'ERROR'}, "No portals or barriers found")
        bm.free()
        return
    
    total_sliced = 0
    
    # Slice normal geometry with portals and barriers.
    for local_point, local_normal, local_boundary_verts in boundaries_data:
        boundary_edges, sliced_faces = clip_mesh_with_boundary(
            bm,
            local_boundary_verts,
            local_normal
        )
        
        total_sliced += sliced_faces
    
    # Remove any face without area.
    small_faces = [
        face
        for face in bm.faces
        if face.calc_area() < 0.000001
    ]
    
    bmesh.ops.delete(
        bm,
        geom=small_faces,
        context='FACES'
    )
    
    bm.to_mesh(main_obj.data)
    main_obj.data.update()
    bm.free()
    
    report(
        {'INFO'},
        f"Sliced {total_sliced} face(s) with boundaries"
    )
    
    return boundaries_data


def debug_select_boundaries(
    main_obj,
    boundary_edges,
    report
):
    bpy.context.view_layer.objects.active = main_obj
    bpy.ops.object.mode_set(mode='EDIT')
    
    bm_edit = bmesh.from_edit_mesh(main_obj.data)
    bm_edit.edges.ensure_lookup_table()
    bm_edit.faces.ensure_lookup_table()
    
    for edge in bm_edit.edges:
        edge.select = False
    
    for face in bm_edit.faces:
        face.select = False
    
    bpy.ops.mesh.select_mode(type='EDGE')
    
    boundary_edge_indices = {
        edge.index for edge in boundary_edges
    }
    
    for index in boundary_edge_indices:
        if index < len(bm_edit.edges):
            bm_edit.edges[index].select = True
    
    bm_edit.select_flush(True)
    bmesh.update_edit_mesh(main_obj.data)
    
    bpy.ops.object.mode_set(mode='EDIT')
    
    report(
        {'INFO'},
        f"Debug: Selected {len(boundary_edges)} boundary edge(s)"
    )


def separate_faces(
    main_obj,
    portal_coll,
    barrier_coll,
    selected_only,
    separate_liquids,
    report
):
    boundaries_data = slice_with_boundaries(
        main_obj,
        portal_coll,
        barrier_coll,
        selected_only,
        report
    )
    
    bm = bmesh.new()
    bm.from_mesh(main_obj.data)
    bm.faces.ensure_lookup_table()
    bm.edges.ensure_lookup_table()
    
    # Collect the edges that touches the portal and the faces that are coplanar to it
    
    boundary_edges = set()
    
    for local_point, local_normal, local_boundary_verts in boundaries_data:
        
        for edge in bm.edges:
            if not edge_lies_on_boundary(
                edge,
                local_point,
                local_normal,
                local_boundary_verts
            ):
                continue
            
            boundary_edges.add(edge)
    
    
    if separate_liquids:
        # Collect all faces that belong to liquid faces
        liquid_faces = {
            face for face in bm.faces
            if is_liquid_face(face, main_obj)
        }
        
        # Collect all edges that belong to liquid faces and add them as boundaries
        for liquid_face in liquid_faces:
            for edge in liquid_face.edges:
                if any(
                    linked_face not in liquid_faces
                    for linked_face in edge.link_faces
                ):
                    boundary_edges.add(edge)
    
    # Add edge-only barriers.
    for barrier in barrier_coll.objects:
        if barrier.type != 'MESH':
            continue
        if barrier.map_props.map_type != 'BARRIER':
            continue
        if selected_only and not barrier.select_get():
            continue
        if barrier.data.polygons:
            continue
        
        matrix = main_obj.matrix_world.inverted() @ barrier.matrix_world
        
        for barrier_edge in barrier.data.edges:
            v0 = matrix @ barrier.data.vertices[barrier_edge.vertices[0]].co
            v1 = matrix @ barrier.data.vertices[barrier_edge.vertices[1]].co
            
            for edge in bm.edges:
                edge_v0 = edge.verts[0].co
                edge_v1 = edge.verts[1].co
                
                if (
                    (edge_v0 - v0).length < 0.0001 and
                    (edge_v1 - v1).length < 0.0001
                ) or (
                    (edge_v0 - v1).length < 0.0001 and
                    (edge_v1 - v0).length < 0.0001
                ):
                    boundary_edges.add(edge)
                    break
    
    if DEBUG:
        debug_select_boundaries(
            main_obj,
            boundary_edges,
            report
        )
        return
    
    # Floodfill using portal/barrier faces and edge-only barriers as walls.
    
    visited = set()
    components = []
    
    for face in bm.faces:
        if face in visited:
            continue
        
        component = []
        queue = [face]
        visited.add(face)
        found_boundary = False
        
        while queue:
            current_face = queue.pop()
            component.append(current_face)
            
            for edge in current_face.edges:
                
                if edge in boundary_edges:
                    found_boundary = True
                    continue
                
                # Normal edge.
                for linked_face in edge.link_faces:
                    if linked_face != current_face and linked_face not in visited:
                        visited.add(linked_face)
                        queue.append(linked_face)
        
        if found_boundary:
            components.append(component)
    
    # Merge the liquid faces with the sectors below them and assign the sector type
    
    if separate_liquids:
        # Create the component AABB's
        component_bounds = {}
        
        for component in components:
            min_corner = Vector((
                float('inf'),
                float('inf'),
                float('inf')
            ))
            
            max_corner = Vector((
                float('-inf'),
                float('-inf'),
                float('-inf')
            ))
            
            for face in component:
                for vertex in face.verts:
                    position = vertex.co
                    
                    min_corner.x = min(min_corner.x, position.x)
                    min_corner.y = min(min_corner.y, position.y)
                    min_corner.z = min(min_corner.z, position.z)
                    
                    max_corner.x = max(max_corner.x, position.x)
                    max_corner.y = max(max_corner.y, position.y)
                    max_corner.z = max(max_corner.z, position.z)
            
            component_bounds[id(component)] = (
                min_corner,
                max_corner
            )
        
        liquid_components = []
        
        for component in components:
            # If the first face is a liquid this is a liquid component
            if is_liquid_face(component[0], main_obj):
                liquid_components.append(component)
        
        component_types = {}
        liquid_types = get_liquid_types()
        
        for liquid_component in liquid_components:
            # Determine the sector type from the liquid material.
            sector_type = 'NONE'
            material = main_obj.data.materials[liquid_component[0].material_index]
            
            for liquid_name, liquid_data in liquid_types.items():
                if material.name.lower().startswith('*' + liquid_name):
                    sector_type = liquid_data["sector_type"]
                    break
            
            min_corner, max_corner = component_bounds[id(liquid_component)]
            
            # Calculate the midpoint and normal for the liquid component
            liquid_center = (min_corner + max_corner) * 0.5
            liquid_normal = liquid_component[0].normal
            
            # Find the component on the opposite side of the normal
            target_component = None
            
            for component in components:
                if component is liquid_component:
                    continue
                
                min_corner, max_corner = component_bounds[id(component)]
                
                # If the liquid midpoint touches the component AABB and is above it
                if (
                    min_corner.x <= liquid_center.x <= max_corner.x and
                    min_corner.y <= liquid_center.y <= max_corner.y and
                    min_corner.z <= liquid_center.z <= max_corner.z
                ):
                    component_center = (min_corner + max_corner) * 0.5
                    
                    # Check which side of the liquid surface the component is on.
                    direction = component_center - liquid_center
                    
                    if direction.dot(liquid_normal) < 0:
                        target_component = component
                        break
            
            # Add the liquid component to the component below it and assign the sector type
            if target_component is not None:
                target_component.extend(liquid_component)
                component_types[id(target_component)] = sector_type
                components.remove(liquid_component)
    
    # extract the faces into new objects
    
    created = []
    
    for i, component in enumerate(components):
        sector_type = component_types.get(id(component), 'NONE')
        
        bm_sector = bmesh.new()
        vert_map = {}
        
        uv_src = bm.loops.layers.uv.active
        uv_dst = bm_sector.loops.layers.uv.new() if uv_src else None
        
        for face in component:
            verts = []
            for v in face.verts:
                if v not in vert_map:
                    vert_map[v] = bm_sector.verts.new(v.co)
                verts.append(vert_map[v])
            try:
                new_face = bm_sector.faces.new(verts)
                new_face.material_index = face.material_index
                
                if uv_src:
                    for src_loop, dst_loop in zip(face.loops, new_face.loops):
                        dst_loop[uv_dst].uv = src_loop[uv_src].uv
            except ValueError:
                pass
        
        mesh = bpy.data.meshes.new(f"{main_obj.name}_sector_{i}")
        bm_sector.to_mesh(mesh)
        bm_sector.free()
        
        for mat in main_obj.data.materials:
            mesh.materials.append(mat)
        
        sector_obj = bpy.data.objects.new(mesh.name, mesh)
        sector_obj.map_props.map_type = 'SECTOR'
        sector_obj.sector_props.sector_type = sector_type
        sector_obj.matrix_world = main_obj.matrix_world.copy()
        main_obj.users_collection[0].objects.link(sector_obj)
        created.append(sector_obj)
        
        # remove the separated faces from the main mesh
        
        bmesh.ops.delete(
            bm,
            geom=component,
            context='FACES'
        )
    
    bm.to_mesh(main_obj.data)
    main_obj.data.update()
    bm.free()
    
    report({'INFO'}, f"Created {len(created)} sector(s)")
    return created


def assign_bounds_and_priorities(obj, report):
    sectors = [
        obj
        for obj in bpy.data.objects
        if obj.type == 'MESH' and
        obj.map_props.map_type == 'SECTOR'
    ]
    
    for sector in sectors:
        min_corner = Vector((
            float('inf'),
            float('inf'),
            float('inf')
        ))
        
        max_corner = Vector((
            float('-inf'),
            float('-inf'),
            float('-inf')
        ))
        
        for vertex in sector.data.vertices:
            position = sector.matrix_world @ vertex.co
            
            min_corner.x = min(min_corner.x, position.x)
            min_corner.y = min(min_corner.y, position.y)
            min_corner.z = min(min_corner.z, position.z)
            
            max_corner.x = max(max_corner.x, position.x)
            max_corner.y = max(max_corner.y, position.y)
            max_corner.z = max(max_corner.z, position.z)
        
        sector.sector_props.min = min_corner
        sector.sector_props.max = max_corner
        sector.sector_props.has_aabb = True
    
    # Sort sectors from smallest AABB volume to largest.
    sectors.sort(
        key=lambda sector: (
            (sector.sector_props.max[0] - sector.sector_props.min[0]) *
            (sector.sector_props.max[1] - sector.sector_props.min[1]) *
            (sector.sector_props.max[2] - sector.sector_props.min[2])
        )
    )
    
    # Assign the lowest possible priority based only on overlapping AABBs.
    processed_sectors = []
    
    for sector in sectors:
        priority = 0
        
        for other in processed_sectors:
            if (
                sector.sector_props.min[0] < other.sector_props.max[0] and
                sector.sector_props.max[0] > other.sector_props.min[0] and
                sector.sector_props.min[1] < other.sector_props.max[1] and
                sector.sector_props.max[1] > other.sector_props.min[1] and
                sector.sector_props.min[2] < other.sector_props.max[2] and
                sector.sector_props.max[2] > other.sector_props.min[2]
            ):
                priority = max(priority, other.sector_props.priority + 1)
        
        sector.sector_props.priority = priority
        processed_sectors.append(sector)
    
    report(
        {'INFO'},
        f"Assigned AABB's and priorities to {len(sectors)} sector(s)"
    )


class sector_properties(bpy.types.PropertyGroup):
    has_pvs: bpy.props.BoolProperty(
        name="Has PVS",
        default=False
    )
    
    has_portals: bpy.props.BoolProperty(
        name="Has Portals",
        default=False
    )
    
    priority: bpy.props.IntProperty(
        name="Priority",
        default=0,
        min=0
    )
    
    has_aabb: bpy.props.BoolProperty(
        name="Has AABB",
        default=False
    )
    
    min: bpy.props.FloatVectorProperty(
        name="Min AABB",
        size=3
    )
    
    max: bpy.props.FloatVectorProperty(
        name="Max AABB",
        size=3
    )
    
    sector_type: bpy.props.StringProperty(
        name="Type",
        default="NONE"
    )


class portal_properties(bpy.types.PropertyGroup):
    main_obj: bpy.props.PointerProperty(
        name="Main Object",
        type=bpy.types.Object,
        description="Mesh whose faces will be separated"
    )
    portal_collection: bpy.props.PointerProperty(
        name="Portal Collection",
        type=bpy.types.Collection,
        description="Collection of portals"
    )
    barrier_collection: bpy.props.PointerProperty(
        name="Barrier Collection",
        type=bpy.types.Collection,
        description="Collection of barriers"
    )
    selected_only: bpy.props.BoolProperty(
        name="Use Only Selected Portals",
        default=False,
        description="When enabled, process only the portals that are selected in the viewport"
    )
    separate_liquids: bpy.props.BoolProperty(
        name="Separate Liquids",
        default=True,
        description="Separate sectors at liquid boundaries"
    )
    sector_0: bpy.props.PointerProperty(
        type=bpy.types.Object
    )
    sector_1: bpy.props.PointerProperty(
        type=bpy.types.Object
    )


class FACEGRAB_OT_slice_with_boundaries(bpy.types.Operator):
    bl_idname = "facegrab.slice_with_boundaries"
    bl_label = "Slice Faces With Boundaries"
    bl_options = {'REGISTER', 'UNDO'}
    
    @classmethod
    def poll(cls, context):
        return context.mode == 'OBJECT'
    
    def execute(self, context):
        props = context.scene.portal_props
        main_obj = props.main_obj
        portal_coll = props.portal_collection
        barrier_coll = props.barrier_collection
        
        if not main_obj or main_obj.type != 'MESH':
            self.report({'ERROR'}, "Main object must be a mesh")
            return {'CANCELLED'}
        
        if not portal_coll and not barrier_coll:
            self.report({'ERROR'}, "Portal or barrier collection must be set")
            return {'CANCELLED'}
        
        if not DEBUG:
            slice_with_boundaries(
                main_obj,
                portal_coll,
                barrier_coll,
                props.selected_only,
                self.report,
            )
        else:
            test_select_affected_faces(
                main_obj,
                portal_coll,
                barrier_coll,
                props.selected_only,
                self.report,
            )
        
        return {'FINISHED'}


class FACEGRAB_OT_separate_sectors(bpy.types.Operator):
    bl_options = {'REGISTER', 'UNDO'}
    bl_idname = "facegrab.separate_sectors"
    bl_label = "Separate Faces With Boundaries Into Sectors"
    bl_description = "Move faces from the main object that are located within portals and barriers into new objects"
    
    @classmethod
    def poll(cls, context):
        return context.mode == 'OBJECT'
    
    def execute(self, context):
        props = context.scene.portal_props
        main_obj = props.main_obj
        portal_coll = props.portal_collection
        barrier_coll = props.barrier_collection
        
        if not main_obj or main_obj.type != 'MESH':
            self.report({'ERROR'}, "Main object must be a mesh")
            return {'CANCELLED'}
        
        if not portal_coll and not barrier_coll:
            self.report({'ERROR'}, "Portal or barrier collection not set")
            return {'CANCELLED'}
        
        separate_faces(
            main_obj,
            portal_coll,
            barrier_coll,
            props.selected_only,
            props.separate_liquids,
            self.report
        )
        
        return {'FINISHED'}


class FACEGRAB_OT_add_portal(bpy.types.Operator):
    bl_idname = "facegrab.add_portal"
    bl_label = "Add Portal"
    
    def execute(self, context):
        if context.mode != 'EDIT_MESH':
            self.report({'ERROR'}, "Select a face in Edit Mode")
            return {'CANCELLED'}
        
        main_obj = context.object
        
        bpy.ops.mesh.separate(type='SELECTED')
        bpy.ops.object.mode_set(mode='OBJECT')
        
        portal = next(
            obj for obj in bpy.context.selected_objects
            if obj != main_obj
        )
        
        portal.name = "Portal"
        
        coll = bpy.data.collections.get("Portals")
        
        if coll is None:
            coll = bpy.data.collections.new("Portals")
            context.scene.collection.children.link(coll)
        
        for c in portal.users_collection:
            c.objects.unlink(portal)
        
        coll.objects.link(portal)
        
        mat = get_portal_material()
        
        portal.data.materials.clear()
        portal.data.materials.append(mat)
        portal.map_props.map_type = 'PORTAL'
        
        self.report({'INFO'}, "Portal created")
        return {'FINISHED'}


class FACEGRAB_OT_add_barrier(bpy.types.Operator):
    bl_idname = "facegrab.add_barrier"
    bl_label = "Add Barrier"
    
    def execute(self, context):
        if context.mode != 'EDIT_MESH':
            self.report({'ERROR'}, "Select a face in Edit Mode")
            return {'CANCELLED'}
        
        main_obj = context.object
        
        bpy.ops.mesh.separate(type='SELECTED')
        bpy.ops.object.mode_set(mode='OBJECT')
        
        barrier = next(
            obj for obj in context.selected_objects
            if obj != main_obj
        )
        
        barrier.name = "Barrier"
        
        coll = bpy.data.collections.get("Barriers")
        
        if coll is None:
            coll = bpy.data.collections.new("Barriers")
            context.scene.collection.children.link(coll)
        
        for collection in barrier.users_collection:
            collection.objects.unlink(barrier)
        
        coll.objects.link(barrier)
        
        mat = get_portal_material()
        
        barrier.data.materials.clear()
        barrier.data.materials.append(mat)
        barrier.map_props.map_type = 'BARRIER'
        
        self.report({'INFO'}, "Barrier created")
        return {'FINISHED'}


class FACEGRAB_OT_assign_sector_bounds_and_priorities(bpy.types.Operator):
    bl_idname = "facegrab.assign_sector_bounds_and_priorities"
    bl_label = "Assign AABB's and Priorities"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        assign_bounds_and_priorities(
            obj,
            self.report
        )
        
        return {'FINISHED'}


class FACEGRAB_OT_assign_portals_and_pvs(bpy.types.Operator):
    bl_idname = "facegrab.assign_portals_and_pvs"
    bl_label = "Assign Portals and PVS to Sectors"
    
    def execute(self, context):
        sectors = [
            obj for obj in bpy.data.objects
            if obj.map_props.map_type == 'SECTOR'
        ]
        
        portals = [
            obj for obj in bpy.data.objects
            if obj.map_props.map_type == 'PORTAL'
        ]
        
        barriers = [
            obj for obj in bpy.data.objects
            if obj.map_props.map_type == 'BARRIER'
        ]
        
        liquid_types = get_liquid_types()
        
        liquid_sectors = [
            sector for sector in sectors
            if sector.sector_props.sector_type in liquid_types
        ]
        
        if not sectors:
            self.report({'WARNING'}, "No sectors found in scene")
            return {'CANCELLED'}
        
        if not portals and not barriers:
            self.report({'WARNING'}, "No portals or barriers found in scene")
            return {'CANCELLED'}
        
        # Check if AABBs have been assigned to sectors
        if any(not sector.sector_props.has_aabb for sector in sectors):
            self.report({'ERROR'}, "Sector AABBs are not set. Run 'Assign Sector AABB's first!")
            return {'CANCELLED'}
        
        # Clear the existing portals
        for portal in portals:
            portal.portal_props.sector_0 = None
            portal.portal_props.sector_1 = None
        
        epsilon = 0.01
        
        for portal in portals:
            if not portal.data.polygons:
                continue
            
            center = portal.matrix_world @ portal.data.polygons[0].center
            matching_sectors = []
            
            for sector in sectors:
                min_corner = Vector(sector.sector_props.min)
                max_corner = Vector(sector.sector_props.max)
                
                inside = (
                    min_corner.x - epsilon <= center.x <= max_corner.x + epsilon and
                    min_corner.y - epsilon <= center.y <= max_corner.y + epsilon and
                    min_corner.z - epsilon <= center.z <= max_corner.z + epsilon
                )
                
                if inside:
                    matching_sectors.append(sector)
            
            matching_sectors.sort(
                key=lambda sector: sector.sector_props.priority
            )
            
            if matching_sectors:
                portal.portal_props.sector_0 = matching_sectors[0]
            
            if len(matching_sectors) > 1:
                portal.portal_props.sector_1 = matching_sectors[1]
        
        # Clear the existing PVS
        for sector in sectors:
            sector.pvs_objects.clear()
        
        for barrier in barriers:
            if not barrier.data.polygons:
                continue
            
            center = barrier.matrix_world @ barrier.data.polygons[0].center
            matching_sectors = []
            
            for sector in sectors:
                min_corner = Vector(sector.sector_props.min)
                max_corner = Vector(sector.sector_props.max)
                
                inside = (
                    min_corner.x - epsilon <= center.x <= max_corner.x + epsilon and
                    min_corner.y - epsilon <= center.y <= max_corner.y + epsilon and
                    min_corner.z - epsilon <= center.z <= max_corner.z + epsilon
                )
                
                if inside:
                    matching_sectors.append(sector)
            
            matching_sectors.sort(
                key=lambda sector: sector.sector_props.priority
            )
            
            # Store every touching sector on each other
            for target in matching_sectors:
                for other in matching_sectors:
                    if target == other:
                        continue
                    
                    item = target.pvs_objects.add()
                    item.obj = other
        
        # Assign the sectors with liquids PVS
        
        for liquid_sector in liquid_sectors:
            liquid_data = liquid_types[liquid_sector.sector_props.sector_type]

            if not liquid_data["is_semitransparent"]:
                continue
            
            liquid_min = Vector(liquid_sector.sector_props.min)
            liquid_max = Vector(liquid_sector.sector_props.max)
            
            liquid_top_center = Vector((
                (liquid_min.x + liquid_max.x) * 0.5,
                (liquid_min.y + liquid_max.y) * 0.5,
                liquid_max.z
            ))
            
            for other_sector in sectors:
                if other_sector == liquid_sector:
                    continue
                
                other_min = Vector(other_sector.sector_props.min)
                other_max = Vector(other_sector.sector_props.max)
                
                liquid_touches = False
                
                # If the liquid top center touches the sector and they have the same priority
                if (
                    other_sector.sector_props.priority == liquid_sector.sector_props.priority and
                    other_min.x <= liquid_top_center.x <= other_max.x and
                    other_min.y <= liquid_top_center.y <= other_max.y and
                    other_min.z == liquid_top_center.z
                ):
                    liquid_touches = True
                    break
                
                if not liquid_touches:
                    continue
                
                # Don't add a PVS if a portal already connects the sectors.
                if any(
                    (
                        portal.portal_props.sector_0 == liquid_sector and
                        portal.portal_props.sector_1 == other_sector
                    ) or (
                        portal.portal_props.sector_0 == other_sector and
                        portal.portal_props.sector_1 == liquid_sector
                    )
                    for portal in portals
                ):
                    continue
                
                # Don't add a duplicate PVS.
                if not any(item.obj == other_sector for item in liquid_sector.pvs_objects):
                    item = liquid_sector.pvs_objects.add()
                    item.obj = other_sector
                
                if not any(item.obj == liquid_sector for item in other_sector.pvs_objects):
                    item = other_sector.pvs_objects.add()
                    item.obj = liquid_sector
        
        sector.sector_props.has_pvs = bool(sector.pvs_objects)
        sector.sector_props.has_portals = bool(sector.portal_objects)
        
        self.report({'INFO'}, "Assigned portals and pvs")
        return {'FINISHED'}


class FACEGRAB_OT_assign_entities_to_sectors(bpy.types.Operator):
    bl_idname = "facegrab.assign_entities_to_sectors"
    bl_label = "Assign Entities to Sectors"
    
    def execute(self, context):
        sectors = [
            obj for obj in bpy.data.objects
            if obj.map_props.map_type == 'SECTOR'
        ]
        
        entities = [
            obj for obj in bpy.data.objects
            if obj.map_props.map_type == 'ENTITY'
        ]
        
        if not sectors:
            self.report({'WARNING'}, "No sectors found in scene")
            return {'CANCELLED'}
        
        if not entities:
            self.report({'WARNING'}, "No entities found in scene")
            return {'CANCELLED'}
        
        # Check if AABBs have been assigned to sectors
        if any(not sector.sector_props.has_aabb for sector in sectors):
            self.report({'ERROR'}, "Sector AABBs are not set. Run 'Assign Sector AABB's first!")
            return {'CANCELLED'}
        
        epsilon = 0.01
        
        for entity in entities:
            entity.entity_props.sector = None
            
            center = entity.matrix_world.translation
            matching_sectors = []
            
            for sector in sectors:
                min_corner = Vector(sector.sector_props.min)
                max_corner = Vector(sector.sector_props.max)
                
                inside = (
                    min_corner.x - epsilon <= center.x <= max_corner.x + epsilon and
                    min_corner.y - epsilon <= center.y <= max_corner.y + epsilon and
                    min_corner.z - epsilon <= center.z <= max_corner.z + epsilon
                )
                
                if inside:
                    matching_sectors.append(sector)
            
            matching_sectors.sort(
                key=lambda sector: sector.sector_props.priority
            )
            
            if matching_sectors:
                entity.entity_props.sector = matching_sectors[0]
        
        self.report({'INFO'}, "Assigned entities")
        return {'FINISHED'}


class FACEGRAB_PT_sector_panel(bpy.types.Panel):
    bl_label = "Sectors"
    bl_idname = "FACEGRAB_PT_sector_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Sectors'
    
    def draw(self, context):
        layout = self.layout
        props = context.object.portal_props
        
        layout.prop(props, "main_obj")
        layout.prop(props, "portal_collection")
        layout.prop(props, "barrier_collection")
        layout.prop(props, "selected_only")
        layout.prop(props, "separate_liquids")
        layout.operator("facegrab.add_portal", text="Add Portal")
        layout.operator("facegrab.add_barrier", text="Add Barrier")
        layout.operator("facegrab.slice_with_boundaries", text="Slice With Sector Boundaries")
        layout.operator("facegrab.separate_sectors", text="Separate Faces Into Sectors")
        layout.operator("object.assign_sector_bounds_and_priorities")
        layout.operator("facegrab.assign_portals_and_pvs", text="Assign Portals and PVS")
        layout.operator("facegrab.assign_entities_to_sectors", text="Assign Entities to Sectors")


classes = (
    sector_properties,
    portal_properties,
    FACEGRAB_OT_slice_with_boundaries,
    FACEGRAB_OT_separate_sectors,
    FACEGRAB_OT_add_portal,
    FACEGRAB_OT_add_barrier,
    FACEGRAB_OT_assign_sector_bounds_and_priorities,
    FACEGRAB_OT_assign_portals_and_pvs,
    FACEGRAB_OT_assign_entities_to_sectors,
    FACEGRAB_PT_sector_panel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Object.sector_props = bpy.props.PointerProperty(
        type=sector_properties
    )
    
    bpy.types.Object.portal_props = bpy.props.PointerProperty(
        type=portal_properties
    )


def unregister():
    del bpy.types.Object.sector_props
    del bpy.types.Object.portal_props
    
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)