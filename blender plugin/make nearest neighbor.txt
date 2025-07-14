import bpy

# Run through all materials of the current blend file
for mat in bpy.data.materials:
    # If the material has a node tree
    if mat.node_tree:
        # Run through all nodes
        for node in mat.node_tree.nodes:
            # If the node type is texture 
            if node.type == 'TEX_IMAGE':
                # Set the interpolation -> Linear, Closest, Cubic, Smart
                node.interpolation = 'Closest'