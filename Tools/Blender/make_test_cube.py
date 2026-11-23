"""
모델 불러오기를 시험하는 큐브를 만든다. 면마다 다른 글자가 적혀 있어 좌우가 뒤집히거나 면 방향이 틀리면 바로 보인다.

    blender --background --python Tools/Blender/make_test_cube.py

결과는 Assets/Models/TestCube/TestCube.gltf, .bin, TestCube.png다.
"""

import os

import bpy

PROJECT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FOLDER = os.path.join(PROJECT, "Assets", "Models", "TestCube")

# 면의 법선 방향마다 텍스처의 어느 칸(4x2)을 쓰는지. 블렌더는 Z가 위고, 앞은 -Y다.
CELLS = {
    (0, -1, 0): (0, 1), (0, 1, 0): (1, 1), (-1, 0, 0): (2, 1),
    (1, 0, 0): (3, 1), (0, 0, 1): (0, 0), (0, 0, -1): (1, 0),
}


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    cube = bpy.context.active_object
    cube.name = "TestCube"
    mesh = cube.data
    layer = mesh.uv_layers.active or mesh.uv_layers.new()
    for face in mesh.polygons:
        normal = tuple(int(round(value)) for value in face.normal)
        column, row = CELLS[normal]
        # 면을 정면에서 볼 때의 오른쪽과 위쪽 방향
        up = (0, 0, 1) if normal[2] == 0 else (0, 1, 0) if normal[2] > 0 else (0, -1, 0)
        right = (up[1] * normal[2] - up[2] * normal[1], up[2] * normal[0] - up[0] * normal[2], up[0] * normal[1] - up[1] * normal[0])
        for loop in face.loop_indices:
            point = mesh.vertices[mesh.loops[loop].vertex_index].co
            u = sum(point[i] * right[i] for i in range(3)) + 0.5
            v = sum(point[i] * up[i] for i in range(3)) + 0.5
            layer.data[loop].uv = ((column + u) / 4.0, (row + v) / 2.0)

    material = bpy.data.materials.new("TestCube")
    material.use_nodes = True
    shader = next(node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
    picture = material.node_tree.nodes.new("ShaderNodeTexImage")
    picture.image = bpy.data.images.load(os.path.join(FOLDER, "TestCube.png"))
    material.node_tree.links.new(picture.outputs["Color"], shader.inputs["Base Color"])
    mesh.materials.append(material)


def export():
    bpy.ops.export_scene.gltf(
        filepath=os.path.join(FOLDER, "TestCube.gltf"),
        export_format="GLTF_SEPARATE",
        export_keep_originals=True,
        export_yup=True,
    )


build()
export()
