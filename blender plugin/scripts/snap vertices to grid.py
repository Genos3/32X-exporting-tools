import bpy
import bmesh

offset = 0.01

for obj in bpy.data.objects:
    if obj.type != 'MESH':
        continue

    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)

    # Snap vertex positions
    for v in bm.verts:
        v.co.x = round(v.co.x / offset) * offset
        v.co.y = round(v.co.y / offset) * offset
        v.co.z = round(v.co.z / offset) * offset

    # Snap UVs
    uv_layer = bm.loops.layers.uv.active
    if uv_layer:
        for face in bm.faces:
            for loop in face.loops:
                uv = loop[uv_layer].uv
                uv.x = round(uv.x / offset) * offset
                uv.y = round(uv.y / offset) * offset

    bm.to_mesh(me)
    bm.free()
