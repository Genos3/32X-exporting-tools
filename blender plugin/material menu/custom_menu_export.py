import bpy

bl_info = {
  "name": "Export Extra Material Properties",
  "blender": (3, 0, 0),
  "category": "Import-Export",
}

BITFLAG_PROPERTIES = {
  "untextured": 1,
  "backface": 2,
  "billboard_n": 4,
  "billboard_v": 8,
  "animated": 16,
}

class MyMaterialPanel(bpy.types.Panel):
  bl_idname = "VIEW3D_PT_my_material_panel"
  bl_label = "Material Properties"
  bl_space_type = 'VIEW_3D'
  bl_region_type = 'UI'
  bl_category = "Material Props"  # Name of the tab in the N-Panel
  
  def draw(self, context):
    layout = self.layout
    material = context.object.active_material
    
    if material is None:
      return
    
    for prop_name in BITFLAG_PROPERTIES:
      layout.prop(material, prop_name)
    
    layout.operator("export.material_bitflags", text="Export Bitflags")

def calculate_bitflag(material):
  bitflag = 0
  for prop_name, flag in BITFLAG_PROPERTIES.items():
    if getattr(material, prop_name, False):
      bitflag |= flag
  return bitflag

class ExportBitflagsOperator(bpy.types.Operator):
  bl_idname = "export.material_bitflags"
  bl_label = "Export Material Bitflags"
  
  def execute(self, context):
    filepath = bpy.path.abspath("//bitflags.txt")
    materials = bpy.data.materials
    
    with open(filepath, 'w') as f:
      f.write("t")
      for material in materials:
        if not material.is_grease_pencil:
          bitflags = calculate_bitflag(material)
          f.write(f" {bitflags}")
    
    self.report({'INFO'}, f"Bitflags exported to {filepath}")
    return {'FINISHED'}

def register():
  for prop_name in BITFLAG_PROPERTIES:
    setattr(
      bpy.types.Material,
      prop_name,
      bpy.props.BoolProperty(name = prop_name.replace("_", " ").title())
    )
  bpy.utils.register_class(MyMaterialPanel)
  bpy.utils.register_class(ExportBitflagsOperator)

def unregister():
  for prop_name in BITFLAG_PROPERTIES:
    delattr(bpy.types.Material, prop_name)
  bpy.utils.unregister_class(MyMaterialPanel)
  bpy.utils.unregister_class(ExportBitflagsOperator)

if __name__ == "__main__":
  register()
