import bpy
import re

from mathutils import Vector

from .sector_panel import sector_properties


def numeric_suffix(name):
    m = re.search(r'\.(\d+)$', name)
    return int(m.group(1)) if m else 0


def gather_objects_in_collection(coll, out_list):
    sorted_objs = sorted(
        coll.objects,
        key=lambda ob: numeric_suffix(ob.name)
    )
    
    for obj in sorted_objs:
        if obj.hide_get():
            continue
        
        if obj.type == 'MESH':
            out_list.append(obj)
    
    for child in coll.children:
        gather_objects_in_collection(child, out_list)


class pvs_object(bpy.types.PropertyGroup):
    obj: bpy.props.PointerProperty(
        type=bpy.types.Object
    )


class OBJECT_OT_pvs_add_selected(bpy.types.Operator):
    bl_idname = "object.pvs_add_selected"
    bl_label = "Add Selected"
    
    def execute(self, context):
        owner = context.object
        
        if owner.map_props.map_type != 'SECTOR':
            return {'CANCELLED'}
        
        existing = {item.obj for item in owner.sector_props.pvs_objects}
        
        for obj in context.selected_objects:
            if obj == owner or obj in existing:
                continue
            
            if obj.map_props.map_type != 'SECTOR':
                continue
            
            item = owner.sector_props.pvs_objects.add()
            item.obj = obj
        
        return {'FINISHED'}


class OBJECT_OT_pvs_remove(bpy.types.Operator):
    bl_idname = "object.pvs_remove"
    bl_label = "Remove PVS Object"
    
    index: bpy.props.IntProperty()
    
    def execute(self, context):
        context.object.sector_props.pvs_objects.remove(self.index)
        return {'FINISHED'}


class OBJECT_OT_pvs_clear(bpy.types.Operator):
    bl_idname = "object.pvs_clear"
    bl_label = "Delete All"
    
    def execute(self, context):
        context.object.sector_props.pvs_objects.clear()
        return {'FINISHED'}


class OBJECT_OT_portal_add_selected(bpy.types.Operator):
    bl_idname = "object.portal_add_selected"
    bl_label = "Add Selected"
    
    def execute(self, context):
        owner = context.object
        
        if owner.map_props.map_type != 'PORTAL':
            return {'CANCELLED'}
        
        for obj in context.selected_objects:
            if obj == owner:
                continue
            
            if obj.map_props.map_type != 'SECTOR':
                continue
            
            if owner.portal_props.sector_0 == obj:
                continue
            
            if owner.portal_props.sector_1 == obj:
                continue
            
            if owner.portal_props.sector_0 is None:
                owner.portal_props.sector_0 = obj
            
            elif owner.portal_props.sector_1 is None:
                owner.portal_props.sector_1 = obj
            
            else:
                break
        
        return {'FINISHED'}


class OBJECT_OT_portal_remove(bpy.types.Operator):
    bl_idname = "object.portal_remove"
    bl_label = "Remove Sector"
    
    index: bpy.props.IntProperty()
    
    def execute(self, context):
        props = context.object.portal_props
        
        if self.index == 0:
            props.sector_0 = None
        else:
            props.sector_1 = None
        
        return {'FINISHED'}


class OBJECT_OT_portal_clear(bpy.types.Operator):
    bl_idname = "object.portal_clear"
    bl_label = "Delete All"
    
    def execute(self, context):
        props = context.object.portal_props
        
        props.sector_0 = None
        props.sector_1 = None
        
        return {'FINISHED'}


class OBJECT_OT_assign_sector_flags(bpy.types.Operator):
    bl_idname = "object.assign_sector_flags"
    bl_label = "Assign to Selected"
    bl_options = {'REGISTER', 'UNDO'}
    
    def execute(self, context):
        active = context.active_object
        source = active.sector_props
        count = 0
        
        for obj in context.selected_objects:
            if obj.type != 'MESH':
                continue
            
            if obj.map_props.map_type != 'SECTOR':
                continue
            
            props = obj.sector_props
            props.has_pvs = source.has_pvs
            props.has_portals = source.has_portals
            count += 1
        
        self.report({'INFO'}, f"Applied sector flags to {count} sector(s)")
        return {'FINISHED'}


class OBJECT_OT_vsd_export(bpy.types.Operator):
    bl_idname = "object.vsd_export"
    bl_label = "Export VSD"
    
    def execute(self, context):
        scene = context.scene
        
        objs_to_export = []
        gather_objects_in_collection(
            scene.collection,
            objs_to_export
        )
        
        object_ids = {
            obj: i
            for i, obj in enumerate(objs_to_export)
        }
        
        filepath = bpy.path.abspath("//map.vsd")
        
        with open(filepath, "w") as f:
            for obj in objs_to_export:
                obj_id = object_ids[obj]
                
                if obj.map_props.map_type == 'NONE':
                    continue
                
                f.write(f"o {obj_id}\n")
                
                if obj.map_props.map_type == 'SECTOR':
                    f.write("t 1\n")
                    
                    if obj.sector_props.visibility_type == 'PVS':
                        f.write("vt 0\n")
                        
                        ids = []
                        
                        for item in obj.sector_props.pvs_objects:
                            if item.obj:
                                ids.append(str(
                                    object_ids[item.obj]
                                ))
                        
                        if ids:
                            f.write(f"s {' '.join(ids)}\n")
                    
                    else:
                        f.write("vt 1\n")
                
                else:
                    f.write("t 2\n")
                    
                    ids = []
                    
                    if obj.portal_props.sector_0:
                        ids.append(str(
                             object_ids[obj.portal_props.sector_0]
                        ))
                    
                    if obj.portal_props.sector_1:
                        ids.append(str(
                            object_ids[obj.portal_props.sector_1]
                        ))
                    
                    if ids:
                        f.write(f"s {' '.join(ids)}\n")
                
                f.write("\n")
        
        self.report({'INFO'}, f"VSD exported to {filepath}")
        return {'FINISHED'}


class OBJECT_PT_pvs_panel(bpy.types.Panel):
    bl_idname = "VIEW3D_PT_pvs_panel"
    bl_label = "PVS"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "PVS"
    
    def draw(self, context):
        layout = self.layout
        obj = context.object
        
        if obj is None:
            return
        
        layout.label(text=obj.name)
        layout.label(text=f"Type: {obj.map_props.map_type.title()}")
        
        if obj.map_props.map_type == 'SECTOR':
            layout.use_property_split = True
            layout.use_property_decorate = False
            
            layout.prop(obj.sector_props, "has_pvs")
            layout.prop(obj.sector_props, "has_portals")
            layout.operator("object.assign_sector_flags")
            layout.separator()
            layout.prop(obj.sector_props, "priority")
            layout.prop(obj.sector_props, "sector_type")
            
            if obj.sector_props.has_pvs:
                layout.operator("object.pvs_add_selected")
                
                if obj.sector_props.pvs_objects:
                    layout.operator("object.pvs_clear")
                    layout.label(text="Visible Objects")
                
                for i, item in enumerate(obj.sector_props.pvs_objects):
                    if item.obj:
                        row = layout.row()
                        row.label(text=item.obj.name)
                        
                        op = row.operator(
                            "object.pvs_remove",
                            text="",
                            icon='X'
                        )
                        op.index = i
        
        elif obj.map_props.map_type == 'PORTAL':
            layout.operator("object.portal_add_selected")
            
            props = obj.portal_props
            
            if props.sector_0 or props.sector_1:
                layout.operator("object.portal_clear")
                layout.operator("object.vsd_export")
                layout.label(text="Connected Sectors")
            
            for i, sector in enumerate((props.sector_0, props.sector_1)):
                if sector:
                    row = layout.row()
                    
                    row.label(text=sector.name)
                    
                    op = row.operator("object.portal_remove", text="", icon='X')
                    op.index = i


classes = (
    OBJECT_OT_pvs_add_selected,
    OBJECT_OT_pvs_remove,
    OBJECT_OT_pvs_clear,
    OBJECT_OT_portal_add_selected,
    OBJECT_OT_portal_remove,
    OBJECT_OT_portal_clear,
    OBJECT_OT_vsd_export,
    OBJECT_PT_pvs_panel,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)


def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)