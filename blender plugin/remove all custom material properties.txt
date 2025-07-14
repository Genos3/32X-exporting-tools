import bpy

# Iterate over all materials in the current scene
for material in bpy.data.materials:
    # Iterate over each material's custom properties
    for prop_name in list(material.keys()):
        if not prop_name.startswith("_"):  # Avoid removing internal properties
            del material[prop_name]

print("Custom properties removed from all materials.")
