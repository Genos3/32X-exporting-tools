from pathlib import Path

import bpy
import gpu
import math
from mathutils import Vector


EPSILON = 0.0001
LEAF_SIZE = 8

BAKE_MODE_GRAYSCALE = 1

WORKGROUP_SIZE = 32
BATCH_SIZE = 128

TEXTURE_WIDTH = 2048

LIGHT_GRID_CELL_SIZE = 8.0
LIGHT_THRESHOLD = 0.1
LIGHT_POWER_SCALE = 0.01


class Triangle:
    def __init__(self, vertex0, vertex1, vertex2, normal, material):
        self.vertex0 = vertex0
        self.vertex1 = vertex1
        self.vertex2 = vertex2
        self.normal = normal
        self.material = material

        self.min = Vector((
            min(vertex0.x, vertex1.x, vertex2.x),
            min(vertex0.y, vertex1.y, vertex2.y),
            min(vertex0.z, vertex1.z, vertex2.z)
        ))

        self.max = Vector((
            max(vertex0.x, vertex1.x, vertex2.x),
            max(vertex0.y, vertex1.y, vertex2.y),
            max(vertex0.z, vertex1.z, vertex2.z)
        ))

        self.center = (self.min + self.max) * 0.5


class BakeLight:
    def __init__(self, position, color, energy):
        self.position = position
        self.color = color
        self.energy = energy * LIGHT_POWER_SCALE
        self.radius_squared = self.energy / LIGHT_THRESHOLD


class BakeVertex:
    def __init__(
        self,
        obj,
        vertex_index,
        position,
        normal,
        loop_indices
    ):
        self.obj = obj
        self.vertex_index = vertex_index
        self.position = position
        self.normal = normal
        self.loop_indices = loop_indices


class BVHNode:
    def __init__(self, triangles):
        self.triangles = triangles
        self.left = None
        self.right = None

        self.min = Vector((
            min(triangle.min.x for triangle in triangles),
            min(triangle.min.y for triangle in triangles),
            min(triangle.min.z for triangle in triangles)
        ))

        self.max = Vector((
            max(triangle.max.x for triangle in triangles),
            max(triangle.max.y for triangle in triangles),
            max(triangle.max.z for triangle in triangles)
        ))


def collect_triangles(objects):
    triangles = []

    for obj in objects:
        if obj.type != 'MESH':
            continue

        mesh = obj.data
        world_matrix = obj.matrix_world
        normal_matrix = world_matrix.to_3x3().inverted().transposed()

        for polygon in mesh.polygons:
            vertices = polygon.vertices

            if len(vertices) < 3:
                continue

            world_vertices = [
                world_matrix @ mesh.vertices[index].co
                for index in vertices
            ]

            normal = (normal_matrix @ polygon.normal).normalized()

            material = (
                mesh.materials[polygon.material_index]
                if polygon.material_index < len(mesh.materials)
                else None
            )

            if len(world_vertices) == 3:
                triangles.append(
                    Triangle(
                        world_vertices[0],
                        world_vertices[1],
                        world_vertices[2],
                        normal,
                        material
                    )
                )

            elif len(world_vertices) == 4:
                # Quads are split only for ray tracing.
                triangles.append(
                    Triangle(
                        world_vertices[0],
                        world_vertices[1],
                        world_vertices[2],
                        normal,
                        material
                    )
                )

                triangles.append(
                    Triangle(
                        world_vertices[0],
                        world_vertices[2],
                        world_vertices[3],
                        normal,
                        material
                    )
                )

    return triangles


def collect_bake_vertices(objects):
    vertices = []

    for obj in objects:
        if obj.type != 'MESH':
            continue

        mesh = obj.data
        world_matrix = obj.matrix_world
        normal_matrix = world_matrix.to_3x3().inverted().transposed()

        vertex_normals = [
            Vector((0.0, 0.0, 0.0))
            for _ in mesh.vertices
        ]

        for polygon in mesh.polygons:
            normal = (normal_matrix @ polygon.normal).normalized()

            for vertex_index in polygon.vertices:
                vertex_normals[vertex_index] += normal

        # Precompute vertex -> loop mapping so output does not scan
        # every mesh loop for every bake vertex.
        loops_by_vertex = [[] for _ in mesh.vertices]

        for loop in mesh.loops:
            loops_by_vertex[loop.vertex_index].append(loop.index)

        for vertex, normal in zip(mesh.vertices, vertex_normals):
            if normal.length_squared == 0.0:
                continue

            position = world_matrix @ vertex.co
            normal.normalize()

            vertices.append(
                BakeVertex(
                    obj,
                    vertex.index,
                    position,
                    normal,
                    loops_by_vertex[vertex.index]
                )
            )

    return vertices


def collect_lights(scene):
    lights = []

    for obj in scene.objects:
        # Point lights are the only supported type for now.
        if obj.type != 'LIGHT' or obj.hide_render:
            continue

        if obj.data.type != 'POINT':
            continue

        lights.append(
            BakeLight(
                obj.matrix_world.translation,
                Vector(obj.data.color),
                obj.data.energy
            )
        )

    return lights


def build_light_grid(lights, triangles):
    minimum = Vector((
        min(triangle.min.x for triangle in triangles),
        min(triangle.min.y for triangle in triangles),
        min(triangle.min.z for triangle in triangles)
    ))

    maximum = Vector((
        max(triangle.max.x for triangle in triangles),
        max(triangle.max.y for triangle in triangles),
        max(triangle.max.z for triangle in triangles)
    ))

    cell_dimensions = (
        math.floor((maximum.x - minimum.x) / LIGHT_GRID_CELL_SIZE) + 1,
        math.floor((maximum.y - minimum.y) / LIGHT_GRID_CELL_SIZE) + 1,
        math.floor((maximum.z - minimum.z) / LIGHT_GRID_CELL_SIZE) + 1
    )

    total_cells = (
        cell_dimensions[0] *
        cell_dimensions[1] *
        cell_dimensions[2]
    )

    cell_lights = [[] for _ in range(total_cells)]

    def get_cell_index(x, y, z):
        return (
            x +
            y * cell_dimensions[0] +
            z * cell_dimensions[0] * cell_dimensions[1]
        )

    for light_index, light in enumerate(lights):
        radius = math.sqrt(light.radius_squared)

        minimum_cell = (
            max(0, math.floor(
                (light.position.x - radius - minimum.x) /
                LIGHT_GRID_CELL_SIZE
            )),
            max(0, math.floor(
                (light.position.y - radius - minimum.y) /
                LIGHT_GRID_CELL_SIZE
            )),
            max(0, math.floor(
                (light.position.z - radius - minimum.z) /
                LIGHT_GRID_CELL_SIZE
            ))
        )

        maximum_cell = (
            min(cell_dimensions[0] - 1, math.floor(
                (light.position.x + radius - minimum.x) /
                LIGHT_GRID_CELL_SIZE
            )),
            min(cell_dimensions[1] - 1, math.floor(
                (light.position.y + radius - minimum.y) /
                LIGHT_GRID_CELL_SIZE
            )),
            min(cell_dimensions[2] - 1, math.floor(
                (light.position.z + radius - minimum.z) /
                LIGHT_GRID_CELL_SIZE
            ))
        )

        for z in range(minimum_cell[2], maximum_cell[2] + 1):
            for y in range(minimum_cell[1], maximum_cell[1] + 1):
                for x in range(minimum_cell[0], maximum_cell[0] + 1):
                    cell_lights[get_cell_index(x, y, z)].append(light_index)

    light_grid = [(0, 0)] * total_cells
    light_list = []

    offset = 0

    for cell_index, lights_in_cell in enumerate(cell_lights):
        count = len(lights_in_cell)

        light_grid[cell_index] = (offset, count)
        light_list.extend(lights_in_cell)

        offset += count

    return (
        minimum,
        cell_dimensions,
        light_grid,
        light_list
    )


def build_bvh(triangles):
    if not triangles:
        return None

    node = BVHNode(triangles)

    if len(triangles) <= LEAF_SIZE:
        return node

    size = node.max - node.min

    if size.x >= size.y and size.x >= size.z:
        axis = 0
    elif size.y >= size.z:
        axis = 1
    else:
        axis = 2

    triangles.sort(key=lambda triangle: triangle.center[axis])

    middle = len(triangles) // 2

    node.left = build_bvh(triangles[:middle])
    node.right = build_bvh(triangles[middle:])

    node.triangles = None

    return node


def flatten_bvh(root):
    nodes = []
    ordered_triangles = []

    def flatten(node):
        node_index = len(nodes)

        # Reserve this slot before adding children
        nodes.append(None)

        if node.triangles is not None:
            first_triangle = len(ordered_triangles)

            # Reorder triangles directly
            ordered_triangles.extend(node.triangles)

            nodes[node_index] = (
                node.min,
                node.max,
                -1,
                -1,
                first_triangle,
                len(node.triangles)
            )

            return node_index

        left = flatten(node.left)
        right = flatten(node.right)

        nodes[node_index] = (
            node.min,
            node.max,
            left,
            right,
            0,
            0
        )

        return node_index

    root_index = flatten(root)

    return nodes, ordered_triangles, root_index


def get_diffuse_color(triangle):
    if triangle.material is None:
        return (1.0, 1.0, 1.0, 1.0)

    color = triangle.material.diffuse_color

    return (
        color[0],
        color[1],
        color[2],
        1.0
    )


# Packs a flat list of floats/ints into a 2D RGBA32 texture grid.

def create_2d_texture(flat_values):
    texel_count = math.ceil(len(flat_values) / 4)
    height = math.ceil(texel_count / TEXTURE_WIDTH)
    total_scalars = TEXTURE_WIDTH * height * 4

    # Pad array and explicitly cast every value to float
    padded = list(flat_values) + [0.0] * (total_scalars - len(flat_values))
    padded = [float(v) for v in padded]

    buffer = gpu.types.Buffer("FLOAT", len(padded), padded)

    return gpu.types.GPUTexture(
        (TEXTURE_WIDTH, height),
        format="RGBA32F",
        data=buffer,
    )


def pad_to_vec4(data):
    rem = len(data) % 4
    if rem != 0:
        data.extend([0.0] * (4 - rem))
    return data


def pack_triangles(triangles):
    values = []
    for triangle in triangles:
        albedo = get_diffuse_color(triangle)
        # Packed into 4 texels (16 floats) instead of 5
        values.extend(
            (
                triangle.vertex0.x, triangle.vertex0.y, triangle.vertex0.z, albedo[0],
                triangle.vertex1.x, triangle.vertex1.y, triangle.vertex1.z, albedo[1],
                triangle.vertex2.x, triangle.vertex2.y, triangle.vertex2.z, albedo[2],
                triangle.normal.x,  triangle.normal.y,  triangle.normal.z,  albedo[3],
            )
        )
    return values


def pack_bvh(nodes):
    data = []
    for minimum, maximum, left, right, first, count in nodes:
        data.extend(
            (
                minimum.x, minimum.y, minimum.z, 0.0,
                maximum.x, maximum.y, maximum.z, 0.0,
                float(left), float(right), float(first), float(count)
            )
        )
    return data


def pack_vertices(vertices):
    values = []
    for vertex in vertices:
        values.extend(
            (
                vertex.position.x, vertex.position.y, vertex.position.z, 0.0, # .w reserved for roughness
                vertex.normal.x,   vertex.normal.y,   vertex.normal.z,   0.0, # .w reserved for emission
            )
        )
    return values


def pack_lights(lights):
    values = []
    for light in lights:
        values.extend(
            (
                light.position.x,
                light.position.y,
                light.position.z,
                0.0,
                light.color.x,
                light.color.y,
                light.color.z,
                1.0,
                light.energy,
                0.0,
                0.0,
                0.0,
            )
        )
    return values or [0.0, 0.0, 0.0, 0.0]


def pack_light_grid(cells):
    values = []

    for offset, count in cells:
        values.extend((float(offset), float(count)))

    return values or [0.0, 0.0]


def pack_light_list(light_list):
    return [float(light) for light in light_list] or [0.0]


def write_lighting(vertices, lighting):
    for vertex, color in zip(vertices, lighting):
        color = (
            max(0.0, min(1.0, color[0])),
            max(0.0, min(1.0, color[1])),
            max(0.0, min(1.0, color[2])),
            1.0
        )

        color_attribute = vertex.obj.data.color_attributes.get("Lighting")

        if color_attribute is None:
            continue

        # CORNER attributes need the vertex result written to every loop
        # using that vertex.
        for loop_index in vertex.loop_indices:
            color_attribute.data[loop_index].color = color


def create_shader():
    shader_path = Path(__file__).with_name("lighting_bake.comp")
    source = shader_path.read_text(encoding="utf-8")

    compute_source = source.split(
        "// -----------------------------------------------------------------------------\n// Compute shader\n// -----------------------------------------------------------------------------",
        1,
    )[1]

    shader_info = gpu.types.GPUShaderCreateInfo()

    shader_info.typedef_source(
        source.split(
            "// -----------------------------------------------------------------------------\n// Compute shader\n// -----------------------------------------------------------------------------",
            1,
        )[0]
    )

    # 2D Samplers - bvhTriangles removed, indices shifted down
    shader_info.sampler(0, "FLOAT_2D", "triangleData")
    shader_info.sampler(1, "FLOAT_2D", "bvhData")
    shader_info.sampler(2, "FLOAT_2D", "bakeVertexData")
    shader_info.sampler(3, "FLOAT_2D", "lightData")

    # Output shifted down to 4
    shader_info.image(4, "RGBA32F", "FLOAT_2D", "lighting", qualifiers={"WRITE"})

    # Push Constants
    shader_info.push_constant("INT", "texWidth")
    shader_info.push_constant("INT", "vertexCount")
    shader_info.push_constant("INT", "bvhRoot")
    shader_info.push_constant("INT", "bakeMode")
    shader_info.push_constant("INT", "vertexOffset")
    shader_info.push_constant("INT", "lightGridOffset")
    shader_info.push_constant("INT", "lightListOffset")
    shader_info.push_constant("VEC3", "lightGridMinimum")
    shader_info.push_constant("IVEC3", "lightGridDimensions")
    shader_info.push_constant("FLOAT", "lightGridSize")

    shader_info.local_group_size(WORKGROUP_SIZE, 1, 1)
    shader_info.compute_source(compute_source)

    return gpu.shader.create_from_info(shader_info)


def bake_lighting(scene, objects, bake_mode=BAKE_MODE_GRAYSCALE):
    triangles = collect_triangles(objects)
    if not triangles:
        return

    bvh = build_bvh(triangles)
    nodes, ordered_triangles, bvh_root = flatten_bvh(bvh)
    vertices = collect_bake_vertices(objects)
    lights = collect_lights(scene)

    if not vertices:
        return

    (
        light_grid_minimum,
        light_grid_dimensions,
        light_grid,
        light_list
    ) = build_light_grid(lights, ordered_triangles)
    
    print(
        f"Lights: {len(lights)}, "
        f"Grid cells: {len(light_grid)}, "
        f"Grid references: {len(light_list)}, "
        f"Average lights/cell: {len(light_list) / len(light_grid):.1f}"
    )

    triangle_data = pack_triangles(ordered_triangles)
    bvh_data = pack_bvh(nodes)
    vertex_data = pack_vertices(vertices)

    light_data_padded = pad_to_vec4(pack_lights(lights))
    light_grid_padded = pad_to_vec4(pack_light_grid(light_grid))
    light_list_padded = pad_to_vec4(pack_light_list(light_list))

    light_grid_offset = len(light_data_padded) // 4
    light_list_offset = light_grid_offset + (len(light_grid_padded) // 4)

    combined_light_data = light_data_padded + light_grid_padded + light_list_padded

    triangle_texture = create_2d_texture(triangle_data)
    bvh_texture = create_2d_texture(bvh_data)
    vertex_texture = create_2d_texture(vertex_data)
    light_texture = create_2d_texture(combined_light_data)

    output_height = math.ceil(len(vertices) / TEXTURE_WIDTH)
    output_height = max(1, output_height)
    output_texture = gpu.types.GPUTexture((TEXTURE_WIDTH, output_height), format="RGBA32F")

    shader = create_shader()
    shader.bind()

    shader.uniform_sampler("triangleData", triangle_texture)
    shader.uniform_sampler("bvhData", bvh_texture)
    shader.uniform_sampler("bakeVertexData", vertex_texture)
    shader.uniform_sampler("lightData", light_texture)

    shader.uniform_float("lightGridMinimum", light_grid_minimum)
    shader.uniform_int("lightGridDimensions", light_grid_dimensions)
    shader.uniform_float("lightGridSize", LIGHT_GRID_CELL_SIZE)

    shader.image("lighting", output_texture)

    shader.uniform_int("texWidth", TEXTURE_WIDTH)
    shader.uniform_int("vertexCount", len(vertices))
    shader.uniform_int("bvhRoot", bvh_root)
    shader.uniform_int("bakeMode", bake_mode)

    shader.uniform_int("lightGridOffset", light_grid_offset)
    shader.uniform_int("lightListOffset", light_list_offset)

    for batch_start in range(0, len(vertices), BATCH_SIZE):
        batch_count = min(
            BATCH_SIZE,
            len(vertices) - batch_start
        )

        shader.uniform_int("vertexOffset", batch_start)

        group_count = (
            batch_count + WORKGROUP_SIZE - 1
        ) // WORKGROUP_SIZE

        gpu.compute.dispatch(shader, group_count, 1, 1)

    result_data = output_texture.read().to_list()

    lighting = []

    for index in range(len(vertices)):
        y = index // TEXTURE_WIDTH
        x = index % TEXTURE_WIDTH

        try:
            color = result_data[y][x]
        except TypeError:
            color = result_data[index]

        lighting.append(color)

    write_lighting(vertices, lighting)

    print(
        f"GPU ray tracer: {len(ordered_triangles)} triangles, "
        f"{len(vertices)} vertices, "
        f"{len(lights)} lights, "
        f"{len(nodes)} BVH nodes"
    )

    return lighting