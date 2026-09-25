import bpy
import os
import re

bl_info = {
    "name": "Custom OBJ Export",
    "blender": (4, 1, 0),
    "category": "Import-Export",
}

def export_mesh(
    obj,
    depsgraph,
    apply_modifiers,
    f_obj,
    y_up,
    export_normals,
    v_off,
    vt_off,
    vn_off,
    default_used
):
    # choose between evaluated (modifiers applied) or original mesh
    if apply_modifiers:
        eval_obj = obj.evaluated_get(depsgraph)
        mesh = eval_obj.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        mw = eval_obj.matrix_world
    else:
        eval_obj = obj
        mesh = obj.data
        mw = obj.matrix_world
    
    nmat = mw.to_3x3()
    
    # Vertices
    for v in mesh.vertices:
        co_world = mw @ v.co
        x, y, z = co_world.x, co_world.y, co_world.z
        if y_up:
            y, z = z, -y
        f_obj.write(f"v {x:.8f} {y:.8f} {z:.8f}\n")
    
    # Deduplicate UVs
    uv_layer = mesh.uv_layers.active.data if mesh.uv_layers.active else None
    uv_map = {}
    uv_list = []
    if uv_layer:
        for loop in mesh.loops:
            uv = uv_layer[loop.index].uv
            key = (loop.vertex_index, round(uv.x, 8), round(uv.y, 8))
            if key not in uv_map:
                uv_map[key] = len(uv_list) + 1 + vt_off
                uv_list.append((uv.x, uv.y))
        for uv in uv_list:
            f_obj.write(f"vt {uv[0]:.8f} {uv[1]:.8f}\n")
    
    # Normals
    if export_normals:
        for poly in mesh.polygons:
            n_world = nmat @ poly.normal
            nx, ny, nz = n_world.x, n_world.y, n_world.z
            if y_up:
                ny, nz = nz, ny
            f_obj.write(f"vn {nx:.6f} {ny:.6f} {nz:.6f}\n")
    
    # Faces grouped by material
    last_mat_name = None
    for poly in mesh.polygons:
        mi = poly.material_index
        mat = mesh.materials[mi] if mesh.materials and mi < len(mesh.materials) else None
        mat_name = mat.name if mat else "DefaultMaterial"
        if mat is None:
            default_used = True
        
        if mat_name != last_mat_name:
            f_obj.write(f"usemtl {mat_name}\n")
            last_mat_name = mat_name
        
        vn_idx = poly.index + 1 + vn_off if export_normals else 0
        parts = []
        for li in poly.loop_indices:
            loop = mesh.loops[li]
            v_idx = loop.vertex_index + 1 + v_off
            if uv_layer:
                uv = uv_layer[li].uv
                key = (loop.vertex_index, round(uv.x, 8), round(uv.y, 8))
                vt_idx = uv_map[key]
                if export_normals:
                    parts.append(f"{v_idx}/{vt_idx}/{vn_idx}")
                else:
                    parts.append(f"{v_idx}/{vt_idx}")
            else:
                if export_normals:
                    parts.append(f"{v_idx}//{vn_idx}")
                else:
                    parts.append(f"{v_idx}")
        f_obj.write("f " + " ".join(parts) + "\n")
    
    v_off += len(mesh.vertices)
    if uv_layer:
        vt_off += len(uv_list)
    if export_normals:
        vn_off += len(mesh.polygons)
    
    # free evaluated mesh only if we created one
    if apply_modifiers:
        eval_obj.to_mesh_clear()
    
    return v_off, vt_off, vn_off, default_used

def export_objects(
    obj_path,
    mtl_path,
    export_objects=True,
    apply_modifiers=True,
    y_up=True,
    selected_only=False,
    export_animation=False,
    export_as_bones=True,
    export_normals=False,
    start_frame=None
):
    scene = bpy.context.scene
    
    def gather_objects_in_collection(coll, out_list, selected_only):
        def numeric_suffix(name):
            # Match a dot followed by 1+ digits at the end
            m = re.search(r'\.(\d+)$', name)
            return int(m.group(1)) if m else 0
        
        # Sort objects by numeric suffix
        sorted_objs = sorted(
            coll.objects,
            key = lambda ob: numeric_suffix(ob.name)
        )
        
        for obj in sorted_objs:
            if obj.hide_get():
                continue
            
            if obj.type == 'MESH' and (not selected_only or obj.select_get()):
                out_list.append(obj)
        
        # Recurse into child collections (keeps Outliner order)
        for child in coll.children:
            gather_objects_in_collection(child, out_list, selected_only)
    
    # Build objs_to_export in Scene Collection order (Outliner order)
    objs_to_export = []
    gather_objects_in_collection(scene.collection, objs_to_export, selected_only)
    
    if not objs_to_export:
        print("No mesh objects to export")
        return False
    
    armature = None
    actions = []
    
    if export_animation:
        for obj in scene.collection.all_objects:
            if obj.type == 'ARMATURE':
                armature = obj
                break
        
        if armature is None:
            print("No armature found")
            return False
        
        for action in bpy.data.actions:
            keyframes = sorted({
                int(key.co.x)
                for fcurve in action.fcurves
                for key in fcurve.keyframe_points
            })
            
            actions.append((action, keyframes))
    
    start_frame = start_frame if start_frame is not None else scene.frame_start
    
    depsgraph = bpy.context.evaluated_depsgraph_get()
    
    v_off, vt_off, vn_off = 0, 0, 0
    default_used = False
    
    with open(obj_path, "w", encoding="utf-8") as f_obj:
        f_obj.write(f"# Custom OBJ Export with Animation\nmtllib {os.path.basename(mtl_path)}\n")
        
        if export_objects:
            if export_animation:
                if export_as_bones:
                    bone_ids = {}
                    
                    for bone in armature.data.bones:
                        if bone.name not in bone_ids:
                            bone_ids[bone.name] = len(bone_ids)
                    
                    # Rest pose
                    f_obj.write("\na rest\n")
                    
                    old_pose = armature.data.pose_position
                    armature.data.pose_position = 'REST'
                    
                    for bone in armature.data.bones:
                        bone_id = bone_ids[bone.name]
                        
                        if bone.parent:
                            parent_id = bone_ids[bone.parent.name]
                        else:
                            parent_id = -1
                        
                        m = bone.matrix_local
                        
                        f_obj.write(
                            f"b {bone_id} {parent_id} {bone.name}\n"
                            f"m "
                            f"{m[0][0]:.8f} {m[0][1]:.8f} {m[0][2]:.8f} {m[0][3]:.8f} "
                            f"{m[1][0]:.8f} {m[1][1]:.8f} {m[1][2]:.8f} {m[1][3]:.8f} "
                            f"{m[2][0]:.8f} {m[2][1]:.8f} {m[2][2]:.8f} {m[2][3]:.8f}\n"
                        )
                    
                    armature.data.pose_position = old_pose
                    
                    # Animations
                    for action, keyframes in actions:
                        armature.animation_data.action = action
                        
                        f_obj.write(f"\na {action.name}\n")
                        
                        for keyframe_id, frame in enumerate(keyframes):
                            scene.frame_set(frame)
                            
                            f_obj.write(f"\nk {keyframe_id}\n")
                            
                            for bone in armature.pose.bones:
                                bone_id = bone_ids[bone.name]
                                
                                if bone.parent:
                                    parent_id = bone_ids[bone.parent.name]
                                else:
                                    parent_id = -1
                                
                                m = bone.matrix_basis
                                
                                f_obj.write(
                                    f"b {bone_id} {parent_id} {bone.name}\n"
                                    f"m "
                                    f"{m[0][0]:.8f} {m[0][1]:.8f} {m[0][2]:.8f} {m[0][3]:.8f} "
                                    f"{m[1][0]:.8f} {m[1][1]:.8f} {m[1][2]:.8f} {m[1][3]:.8f} "
                                    f"{m[2][0]:.8f} {m[2][1]:.8f} {m[2][2]:.8f} {m[2][3]:.8f}\n"
                                )
                    
                    # export objects
                    for obj in objs_to_export:
                        bone_id = -1
                        
                        if obj.parent_type == 'BONE':
                            bone_id = bone_ids[obj.parent_bone]
                        
                        obj_name = f"{obj.name}"
                        
                        f_obj.write(f"\no {obj.name} {bone_id}\n")
                        
                        v_off, vt_off, vn_off, default_used = export_mesh(
                            obj,
                            depsgraph,
                            apply_modifiers,
                            f_obj,
                            y_up,
                            export_normals,
                            v_off,
                            vt_off,
                            vn_off,
                            default_used
                        )
                else:
                    for action, keyframes in actions:
                        armature.animation_data.action = action
                        
                        f_obj.write(f"\na {action.name}\n")
                        
                        for keyframe_id, frame in enumerate(keyframes):
                            scene.frame_set(frame)
                            
                            # Export each object individually
                            for obj in objs_to_export:
                                f_obj.write(f"\no {obj.name}_{keyframe_id}\n")
                                
                                v_off, vt_off, vn_off, default_used = export_mesh(
                                    obj,
                                    depsgraph,
                                    apply_modifiers,
                                    f_obj,
                                    y_up,
                                    export_normals,
                                    v_off,
                                    vt_off,
                                    vn_off,
                                    default_used
                                )
            else:
                # Export each object individually
                for obj in objs_to_export:
                    f_obj.write(f"\no {obj.name}\n")
                    
                    v_off, vt_off, vn_off, default_used = export_mesh(
                        obj,
                        depsgraph,
                        apply_modifiers,
                        f_obj,
                        y_up,
                        export_normals,
                        v_off,
                        vt_off,
                        vn_off,
                        default_used
                    )
        else:
            # Export all objects as a single combined object named after the first object
            if objs_to_export:
                first_obj_name = objs_to_export[0].name
                # add frame suffix only when exporting animation
                if export_animation:
                    f_obj.write(f"\no {first_obj_name}_{keyframe_id}\n")
                else:
                    f_obj.write(f"\no {first_obj_name}\n")
            
            for obj in objs_to_export:
                v_off, vt_off, vn_off, default_used = export_mesh(
                    obj,
                    depsgraph,
                    apply_modifiers,
                    f_obj,
                    y_up,
                    export_normals,
                    v_off,
                    vt_off,
                    vn_off,
                    default_used
                )
    
    # Write MTL
    with open(mtl_path, "w", encoding="utf-8") as f_mtl:
        mats = set()
        for obj in objs_to_export:
            for slot in obj.material_slots:
                mat = slot.material
                name = mat.name if mat else "DefaultMaterial"
                if name in mats:
                    continue
                mats.add(name)
                
                f_mtl.write(f"newmtl {name}\n")
                if mat and mat.use_nodes:
                    bsdf = next((n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                    if bsdf:
                        color = bsdf.inputs['Base Color'].default_value
                        f_mtl.write(f"Kd {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
                        f_mtl.write("Ka 0 0 0\nKs 0 0 0\nd 1.0\n")
                        for link in mat.node_tree.links:
                            if link.to_node == bsdf and link.to_socket.name == "Base Color":
                                from_node = link.from_node
                                if from_node.type == 'TEX_IMAGE' and from_node.image:
                                    image_path = bpy.path.relpath(from_node.image.filepath)
                                    image_path = image_path[2:]  # remove the leading //
                                    image_path = image_path.replace("\\", "/")
                                    f_mtl.write(f"map_Kd {image_path}\n\n")
                        continue
                f_mtl.write("Kd 0.8 0.8 0.8\nKa 0.2 0.2 0.2\nKs 0 0 0\nd 1.0\n")
        
        if default_used and "DefaultMaterial" not in mats:
            f_mtl.write("\nnewmtl DefaultMaterial\nKd 0.8 0.8 0.8\nKa 0.2 0.2 0.2\nKs 0 0 0\nd 1.0\n")
    
    print(f"Exported OBJ: {obj_path}")
    print(f"Exported MTL: {mtl_path}")
    
    return True


class EXPORT_OT_custom_obj(bpy.types.Operator):
    """Export selected meshes to custom OBJ/MTL with animation"""
    bl_idname = "export_scene.custom_obj"
    bl_label = "Export Custom OBJ"

    filepath: bpy.props.StringProperty(subtype="FILE_PATH")
    export_objects: bpy.props.BoolProperty(
        name="Export Objects",
        description="Export each object as a separate OBJ object",
        default=True
    )
    apply_modifiers: bpy.props.BoolProperty(
        name="Apply Modifiers",
        description="Apply all modifiers before export",
        default=True
    )
    y_up: bpy.props.BoolProperty(
        name="Y Up",
        description="Swap Y and Z axes for export",
        default=True
    )
    export_selected: bpy.props.BoolProperty(
        name="Export Selected Only",
        description="Export only selected objects",
        default=False
    )
    export_animation: bpy.props.BoolProperty(
        name="Export Animation",
        description="Export all frames (unchecked = first frame only)",
        default=False
    )
    export_as_bones: bpy.props.BoolProperty(
        name="Export As Bones",
        description="Export armature keyframes instead of mesh snapshots",
        default=True
    )
    export_normals: bpy.props.BoolProperty(
        name="Export Normals",
        description="Include normals in the OBJ file",
        default=False
    )
    
    def execute(self, context):
        obj_path = self.filepath if self.filepath.lower().endswith(".obj") else self.filepath + ".obj"
        mtl_path = os.path.splitext(obj_path)[0] + ".mtl"
        
        scene = context.scene
        start_frame = scene.frame_start if self.export_animation else scene.frame_start
        
        success = export_objects(
            obj_path,
            mtl_path,
            export_objects=self.export_objects,
            apply_modifiers=self.apply_modifiers,
            y_up=self.y_up,
            selected_only=self.export_selected,
            export_animation=self.export_animation,
            export_as_bones=self.export_as_bones,
            export_normals=self.export_normals,
            start_frame=start_frame
        )
        
        if success:
            self.report({'INFO'}, "Object file exported")
        else:
            self.report({'ERROR'}, "Export failed")
        
        return {'FINISHED'}
    
    def invoke(self, context, event):
        if bpy.data.filepath:
            blend_name = os.path.splitext(os.path.basename(bpy.data.filepath))[0]
            blend_dir = os.path.dirname(bpy.data.filepath)
        else:
            blend_name = "untitled"
            blend_dir = os.path.expanduser("~")
        self.filepath = os.path.join(blend_dir, blend_name + ".obj")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_func_export(self, context):
    self.layout.operator(EXPORT_OT_custom_obj.bl_idname, text="Export Custom OBJ (.obj)")


def register():
    bpy.utils.register_class(EXPORT_OT_custom_obj)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(EXPORT_OT_custom_obj)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
