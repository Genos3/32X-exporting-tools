import os
import struct

import bpy
import bmesh

from collections import namedtuple
from mathutils import Matrix, Vector
from pathlib import Path
from random import uniform 

from bpy.props import (
    BoolProperty,
    FloatProperty,
    IntProperty,
    PointerProperty,
    StringProperty,
)

entity_links = {}

REMOVE_DOUBLE_SIDED_FACES = True

BSP_VERSION = 29

LUMP_ENTITIES = 0
LUMP_PLANES = 1
LUMP_TEXTURES = 2
LUMP_VERTICES = 3
LUMP_VISIBILITY = 4
LUMP_NODES = 5
LUMP_TEXINFO = 6
LUMP_FACES = 7
LUMP_LIGHTING = 8
LUMP_CLIPNODES = 9
LUMP_LEAVES = 10
LUMP_MARKSURFACES = 11
LUMP_EDGES = 12
LUMP_SURFEDGES = 13
LUMP_MODELS = 14

NUM_LUMPS = 15

HEADER_STRUCT = struct.Struct("<i")
LUMP_STRUCT = struct.Struct("<ii")

Vertex = namedtuple("Vertex", (
    "x",
    "y",
    "z",
))

Edge = namedtuple("Edge", (
    "v0",
    "v1",
))

Face = namedtuple("Face", (
    "planenum",
    "side",
    "firstedge",
    "numedges",
    "texinfo",
    "style0",
    "style1",
    "style2",
    "style3",
    "lightofs",
))

Model = namedtuple("Model", (
    "mins_x",
    "mins_y",
    "mins_z",
    "maxs_x",
    "maxs_y",
    "maxs_z",
    "origin_x",
    "origin_y",
    "origin_z",
    "headnode0",
    "headnode1",
    "headnode2",
    "headnode3",
    "visleafs",
    "firstface",
    "numfaces",
))

Texture = namedtuple("Texture", (
    "name",
    "width",
    "height",
    "offset0",
    "offset1",
    "offset2",
    "offset3",
))

TexInfo = namedtuple("TexInfo", (
    "s_x",
    "s_y",
    "s_z",
    "s_dist",
    "t_x",
    "t_y",
    "t_z",
    "t_dist",
    "miptex",
    "flags",
))


def read_lumps(file):
    lumps = []
    
    for _ in range(NUM_LUMPS):
        offset, size = LUMP_STRUCT.unpack(file.read(LUMP_STRUCT.size))
        lumps.append((offset, size))
    
    return lumps


def read_lump(file, lump, struct_format):
    offset, size = lump
    
    file.seek(offset)
    
    struct_size = struct.calcsize(struct_format)
    count = size // struct_size
    
    return [
        struct.unpack(struct_format, file.read(struct_size))
        for _ in range(count)
    ]


def read_lump_named(file, lump, struct_format, cls):
    offset, size = lump
    
    file.seek(offset)
    
    struct_size = struct.calcsize(struct_format)
    count = size // struct_size
    
    return [
        cls._make(
            struct.unpack(
                struct_format,
                file.read(struct_size)
            )
        )
        for _ in range(count)
    ]


def read_lump_bytes(file, lump):
    offset, size = lump
    
    file.seek(offset)
    return file.read(size)


def parse_entities(entity_text):
    entities = []
    
    entity = None
    
    for line in entity_text.splitlines():
        line = line.strip()
        
        if line == "{":
            entity = {}
            continue
        
        if line == "}":
            entities.append(entity)
            entity = None
            continue
        
        if entity is None:
            continue
        
        values = line.split('"')
        
        if len(values) >= 4:
            entity[values[1]] = values[3]
    
    return entities


def get_entities(file, lumps):
    entity_text = read_lump_bytes(
        file,
        lumps[LUMP_ENTITIES]
    ).decode("ascii")
    
    return parse_entities(entity_text)


def entity_get_float(entity, key, default=0.0):
    value = entity.get(key)
    
    if value is None:
        return default
    
    return float(value)


def entity_get_vector(entity, key):
    value = entity.get(key)
    
    if value is None:
        return (0.0, 0.0, 0.0)
    
    return tuple(float(v) for v in value.split())


def create_entity_object(
    collection,
    entity,
    global_matrix
):
    obj = bpy.data.objects.new(
        entity["classname"],
        None
    )
    
    obj.empty_display_type = 'PLAIN_AXES'
    obj.empty_display_size = (1.0 / global_matrix.to_scale()[0]) * 0.5
    obj.show_name = True
    
    obj.matrix_world = (
        global_matrix @
        Matrix.Translation(
            entity_get_vector(entity, "origin")
        )
    )
    
    for key, value in entity.items():
        obj[key] = value
    
    collection.objects.link(obj)
    
    return obj


def create_light(context, collection, entity, global_matrix):
    classname = entity.get("classname", "light")
    
    obj_name = entity.get("targetname", classname)
    
    light = bpy.data.lights.new(
        obj_name,
        'POINT'
    )
    
    light_val = entity.get("light", entity.get("_light", 300.0))
    light.energy = float(light_val)
    
    obj = bpy.data.objects.new(obj_name, light)
    obj.show_name = True
    
    obj.matrix_world = (
        global_matrix @
        Matrix.Translation(
            entity_get_vector(entity, "origin")
        )
    )
    
    obj.map_props.map_type = 'ENTITY'
    obj.entity_props.type = classname
    
    # Copy raw BSP key-values to custom properties for exporter compatibility
    for key, value in entity.items():
        obj[key] = value
    
    collection.objects.link(obj)
    
    return obj


def read_vertices(file, lumps):
    # float point[3];
    return read_lump_named(
        file,
        lumps[LUMP_VERTICES],
        "<fff",
        Vertex
    )


def read_edges(file, lumps):
    # unsigned short v[2];
    return read_lump_named(
        file,
        lumps[LUMP_EDGES],
        "<HH",
        Edge
    )


def read_surfedges(file, lumps):
    return read_lump(
        file,
        lumps[LUMP_SURFEDGES],
        "<i"
    )


def read_faces(file, lumps):
    # short planenum;
    # short side;
    # int firstedge;
    # short numedges;
    # short texinfo;
    # byte styles[4];
    # int lightofs;
    return read_lump_named(
        file,
        lumps[LUMP_FACES],
        "<HHiHHBBBBi",
        Face
    )


def read_models(file, lumps):
    # float mins[3];
    # float maxs[3];
    # float origin[3];
    # int headnode[4];
    # int visleafs;
    # int firstface;
    # int numfaces;
    return read_lump_named(
        file,
        lumps[LUMP_MODELS],
        "<9f7i",
        Model
    )


def read_texinfo(file, lumps):
    # float vecs[2][4];
    # int miptex;
    # int flags;
    return read_lump_named(
        file,
        lumps[LUMP_TEXINFO],
        "<8f2i",
        TexInfo
    )


def read_textures(file, lumps):
    offset, size = lumps[LUMP_TEXTURES]
    
    file.seek(offset)
    
    num_textures = struct.unpack("<i", file.read(4))[0]
    
    texture_offsets = struct.unpack(
        f"<{num_textures}i",
        file.read(num_textures * 4)
    )
    
    textures = []
    
    for texture_offset in texture_offsets:
        if texture_offset == -1:
            textures.append(None)
            continue
        
        file.seek(offset + texture_offset)
        
        name = file.read(16).split(b"\0", 1)[0].decode("ascii")
        
        width, height = struct.unpack(
            "<ii",
            file.read(8)
        )
        
        mip_offsets = struct.unpack(
            "<4i",
            file.read(16)
        )
        
        textures.append(
            Texture(
                name,
                width,
                height,
                *mip_offsets
            )
        )
    
    return textures


def read_texture_pixels(file, lumps, texture, texture_index):
    offset, _ = lumps[LUMP_TEXTURES]
    
    file.seek(offset)
    
    num_textures = struct.unpack(
        "<i",
        file.read(4)
    )[0]
    
    texture_offsets = struct.unpack(
        f"<{num_textures}i",
        file.read(num_textures * 4)
    )
    
    texture_offset = texture_offsets[texture_index]
    
    if texture_offset == -1:
        return None
    
    file.seek(offset + texture_offset)
    
    file.seek(16 + 8 + 16, 1)
    
    file.seek(
        offset +
        texture_offset +
        texture.offset0
    )
    
    return file.read(
        texture.width *
        texture.height
    )


def read_palette():
    palette = []
    
    palette_path = Path(__file__).with_name("palette.lmp")
    
    with open(palette_path, "rb") as file:
        data = file.read()
    
    for i in range(0, len(data), 3):
        palette.append((
            data[i] / 255.0,
            data[i + 1] / 255.0,
            data[i + 2] / 255.0,
            1.0
        ))
    
    return palette


def create_texture_image(texture, pixels, palette):
    image = bpy.data.images.new(
        texture.name,
        width=texture.width,
        height=texture.height,
        alpha=False
    )
    
    rgba = []
    for y in reversed(range(texture.height)):   # flip Y
        for x in range(texture.width):
            index = pixels[y * texture.width + x]
            rgba.extend(palette[index])
    
    image.pixels = rgba
    image.pack()
    return image


def create_material(texture, image):
    material = bpy.data.materials.new(texture.name)
    material.use_nodes = True
    material.diffuse_color = [uniform(0.1, 1.0), uniform(0.1, 1.0), uniform(0.1, 1.0), 1.0]
    
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    
    nodes.clear()
    
    output = nodes.new("ShaderNodeOutputMaterial")
    bsdf = nodes.new("ShaderNodeBsdfDiffuse")
    tex = nodes.new("ShaderNodeTexImage")
    
    tex.interpolation = 'Closest'
    tex.image = image
    
    links.new(
        tex.outputs["Color"],
        bsdf.inputs["Color"]
    )
    
    links.new(
        bsdf.outputs["BSDF"],
        output.inputs["Surface"]
    )
    
    material.use_backface_culling = True
    
    return material


def get_face_vertex_indices(face, edges, surfedges):
    first_edge = face.firstedge
    num_edges = face.numedges
    
    vertex_indices = []
    
    for i in range(first_edge, first_edge + num_edges):
        surfedge = surfedges[i][0]
        
        if surfedge >= 0:
            edge = edges[surfedge]
            vertex = edge.v0
        else:
            edge = edges[-surfedge]
            vertex = edge.v1
        
        if not vertex_indices or vertex != vertex_indices[-1]:
            vertex_indices.append(vertex)
    
    return vertex_indices[::-1]


def remove_back_faces(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    
    faces_to_delete = []
    
    for face in bm.faces:
        
        material = None
        
        if face.material_index < len(mesh.materials):
            material = mesh.materials[face.material_index]
        
        if material is None:
            continue
        
        if not material.name.startswith("*"):
            continue
        
        # Remove downward-facing liquid faces.
        if REMOVE_DOUBLE_SIDED_FACES:
            if face.normal.z < 0.0:
                faces_to_delete.append(face)
    
    if faces_to_delete:
        bmesh.ops.delete(
            bm,
            geom=faces_to_delete,
            context='FACES'
        )
    
    bm.to_mesh(mesh)
    mesh.update()
    bm.free()


def average_image_color(image):
    image.pixels.foreach_get(pixels := [0.0] * len(image.pixels))
    
    r = g = b = 0.0
    count = 0
    
    for i in range(0, len(pixels), 4):
        a = pixels[i + 3]
        
        if a < 0.01:
            continue
        
        r += pixels[i]
        g += pixels[i + 1]
        b += pixels[i + 2]
        count += 1
    
    if count == 0:
        return (0.8, 0.8, 0.8, 1.0)
    
    return (
        r / count,
        g / count,
        b / count,
        1.0,
    )


def create_brush_model(
    context,
    collection,
    name,
    global_matrix,
    vertices,
    edges,
    surfedges,
    faces,
    models,
    model_index,
    texinfo,
    textures,
    texture_materials
):
    model = models[model_index]
    
    first_face = model.firstface
    num_faces = model.numfaces
    
    vertex_map = {}
    
    mesh_vertices = []
    mesh_faces = []
    face_vertex_lists = []
    face_materials = []
    
    for i in range(first_face, first_face + num_faces):
        face = faces[i]
        
        face_vertices = get_face_vertex_indices(
            face,
            edges,
            surfedges
        )
        
        if len(face_vertices) < 3:
            continue
        
        mapped_face = []
        
        for vertex_index in face_vertices:
            mapped = vertex_map.get(vertex_index)
            
            if mapped is None:
                mapped = len(mesh_vertices)
                vertex_map[vertex_index] = mapped
                
                mesh_vertices.append(
                    tuple(
                        global_matrix @
                        Vector(vertices[vertex_index])
                    )
                )
            
            mapped_face.append(mapped)
        
        mesh_faces.append(mapped_face)
        face_vertex_lists.append(face_vertices)
        face_materials.append(face.texinfo)
    
    min_x = min(v[0] for v in mesh_vertices)
    min_y = min(v[1] for v in mesh_vertices)
    min_z = min(v[2] for v in mesh_vertices)
    
    max_x = max(v[0] for v in mesh_vertices)
    max_y = max(v[1] for v in mesh_vertices)
    max_z = max(v[2] for v in mesh_vertices)
    
    origin = Vector((
        (min_x + max_x) * 0.5,
        (min_y + max_y) * 0.5,
        (min_z + max_z) * 0.5,
    ))
    
    mesh_vertices = [
        (v[0] - origin.x,
         v[1] - origin.y,
         v[2] - origin.z)
        for v in mesh_vertices
    ]
    
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(
        mesh_vertices,
        [],
        mesh_faces
    )
    mesh.update()
    
    uv_layer = mesh.uv_layers.new(name="UVMap")
    material_indices = {}
    
    for polygon, original_vertices, texinfo_index in zip(
        mesh.polygons,
        face_vertex_lists,
        face_materials
    ):
        tex = texinfo[texinfo_index]
        texture = textures[tex.miptex]
        
        if texture is None:
            continue
        
        for loop_index in polygon.loop_indices:
            original_vertex = original_vertices[
                loop_index - polygon.loop_start
            ]
            
            vertex = vertices[original_vertex]
            
            s = (
                vertex.x * tex.s_x +
                vertex.y * tex.s_y +
                vertex.z * tex.s_z +
                tex.s_dist
            ) / texture.width
            
            t = (
                vertex.x * tex.t_x +
                vertex.y * tex.t_y +
                vertex.z * tex.t_z +
                tex.t_dist
            ) / texture.height
            
            uv_layer.uv[loop_index].vector = (s, -t)
    
    for polygon, texinfo_index in zip(
        mesh.polygons,
        face_materials
    ):
        tex = texinfo[texinfo_index]
        texture = textures[tex.miptex]
        
        if texture is None:
            continue
        
        material = texture_materials.get(texture.name)
        
        if material is None:
            continue
        
        index = material_indices.get(material.name)
        
        if index is None:
            index = len(mesh.materials)
            mesh.materials.append(material)
            material_indices[material.name] = index
        
        polygon.material_index = index
    
    remove_back_faces(mesh)
    
    obj = bpy.data.objects.new(
        name,
        mesh
    )
    
    obj.location = origin
    
    collection.objects.link(obj)
    
    return obj


def get_link_position(obj):
    if obj.type == 'MESH':
        return obj.matrix_world.translation + Vector(obj.dimensions) * 0.5
    return obj.matrix_world.translation


def get_entity_collection(classname):
    if classname.startswith("trigger_"):
        return "Triggers"
    
    if classname.startswith("func_door"):
        return "Doors"
    
    if (
        classname.startswith("func_plat") or
        classname.startswith("func_train")
    ):
        return "Platforms"
    
    if classname == "func_button":
        return "Buttons"
    
    if classname.startswith("monster_"):
        return "Enemies"
    
    if classname.startswith("item_"):
        return "Items"
    
    if classname.startswith("weapon_"):
        return "Weapons"
    
    if classname.startswith("info_"):
        return "Info"
    
    if classname.startswith("misc_"):
        return "Misc"
    
    if classname.startswith("ambient_"):
        return "Ambient"
    
    if classname.startswith("light"):
        return "Lights"
    
    if classname == "path_corner":
        return "Path Corners"
    
    return None


def import_bsp(
    self,
    context,
    filepath,
    global_matrix,
    import_world,
    import_lights,
    import_cameras,
    import_entities,
    create_materials,
    import_textures
):
    # Remove default scene objects.
    for obj in list(bpy.context.scene.objects):
        if obj.type in {'CAMERA', 'LIGHT'} or obj.name == "Cube":
            bpy.data.objects.remove(obj, do_unlink=True)
    
    try:
        file = open(filepath, "rb")
    except OSError as error:
        self.report({'ERROR'}, str(error))
        return {'CANCELLED'}
    
    version = HEADER_STRUCT.unpack(file.read(4))[0]
    
    if version != BSP_VERSION:
        self.report({'ERROR'}, "Not a Quake 1 BSP.")
        file.close()
        return {'CANCELLED'}
    
    lumps = read_lumps(file)
    
    map_name = os.path.splitext(
        os.path.basename(filepath)
    )[0]
    
    entities = get_entities(file, lumps)
    
    vertices = read_vertices(file, lumps)
    edges = read_edges(file, lumps)
    surfedges = read_surfedges(file, lumps)
    faces = read_faces(file, lumps)
    texinfo = read_texinfo(file, lumps)
    models = read_models(file, lumps)
    textures = read_textures(file, lumps)
    
    texture_images = {}
    texture_materials = {}
    
    if import_textures:
        palette = read_palette()
        
        for i, texture in enumerate(textures):
            if texture is None:
                continue
            
            pixels = read_texture_pixels(
                file,
                lumps,
                texture,
                i
            )
            
            texture_images[texture.name] = create_texture_image(
                texture,
                pixels,
                palette
            )
            
            texture_materials[texture.name] = create_material(
                texture,
                texture_images[texture.name]
            )
    
    root = bpy.data.collections.new(map_name)
    context.scene.collection.children.link(root)
    
    collections = {}
    
    for name in (
        "World",
        "Triggers",
        "Doors",
        "Buttons",
        "Platforms",
        "Enemies",
        "Items",
        "Weapons",
        "Misc",
        "Info",
        "Lights",
        "Ambient",
        "Path Corners"
    ):
        collection = bpy.data.collections.new(name)
        root.children.link(collection)
        collections[name] = collection
    
    if import_world:
        create_brush_model(
            context,
            collections["World"],
            map_name,
            global_matrix,
            vertices,
            edges,
            surfedges,
            faces,
            models,
            0,
            texinfo,
            textures,
            texture_materials
        )
    
    entity_objects = {}
    entity_to_object = {}
    
    if import_entities:
        for entity in entities:
            model = entity.get("model")
            
            if model is None or not model.startswith("*"):
                classname = entity["classname"]
                
                if classname.startswith("light"):
                    continue
                
                collection_name = get_entity_collection(classname)
                
                if collection_name is None:
                    continue
                
                entity_object = create_entity_object(
                    collections[collection_name],
                    entity,
                    global_matrix
                )
                
                entity_object.map_props.map_type = 'ENTITY'
                entity_object.entity_props.type = classname
                
                for key, value in entity.items():
                    entity_object[key] = value
                
                entity_to_object[id(entity)] = entity_object
                
                targetname = entity.get("targetname")
                
                if targetname:
                    entity_objects[targetname] = entity_object
                
                continue
            
            model_index = int(model[1:])
            
            if model_index == 0:
                continue
            
            collection_name = get_entity_collection(
                entity["classname"]
            )
            
            if collection_name is None:
                continue
            
            mesh_object = create_brush_model(
                context,
                collections[collection_name],
                entity["classname"],
                global_matrix,
                vertices,
                edges,
                surfedges,
                faces,
                models,
                model_index,
                texinfo,
                textures,
                texture_materials
            )
            
            mesh_object.map_props.map_type = 'ENTITY'
            mesh_object.entity_props.type = entity["classname"]
            mesh_object["classname"] = entity["classname"]
            
            for key, value in entity.items():
                mesh_object[key] = value
            
            # make the triggers semi-transparent
            if entity["classname"].startswith("trigger_"):
                trigger_mat = bpy.data.materials.new("trigger")
                trigger_mat.use_nodes = True
                trigger_mat.blend_method = 'BLEND'
                trigger_mat.shadow_method = 'NONE'
                trigger_mat.diffuse_color = (1.0, 0.5, 0.0, 0.3)
                bsdf = trigger_mat.node_tree.nodes.get("Principled BSDF")
                
                if bsdf:
                    bsdf.inputs["Base Color"].default_value = (1.0, 0.5, 0.0, 1.0)
                    bsdf.inputs["Alpha"].default_value = 0.15
                
                mesh_object.data.materials.clear()
                mesh_object.data.materials.append(trigger_mat)
            
            entity_to_object[id(entity)] = mesh_object
            
            targetname = entity.get("targetname")
            
            if targetname:
                entity_objects[targetname] = mesh_object
    
    if import_lights:
        for entity in entities:
            classname = entity["classname"]
            
            if not classname.startswith("light"):
                continue
            
            light_obj = create_light(
                context,
                collections["Lights"],
                entity,
                global_matrix
            )
            
            # Add the light object to the dictionaries so it can be linked/targeted
            if import_entities:
                entity_to_object[id(entity)] = light_obj
                
                targetname = entity.get("targetname")
                if targetname:
                    entity_objects[targetname] = light_obj
    
    for area in bpy.context.screen.areas:
        if area.ui_type == 'VIEW_3D':
            for space in area.spaces:
                if hasattr(space, 'shading'):
                    space.shading.show_backface_culling = True
                    space.shading.show_specular_highlight = False
    
    file.close()
    return {'FINISHED'}
